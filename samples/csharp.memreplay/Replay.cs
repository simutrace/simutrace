using SimuTrace;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;

namespace csharp.memreplay
{
    [Serializable]
    internal class SimutraceException : Exception
    {
        public SimutraceException() :
            base(_getMessage()) { }

        static private string _getMessage()
        {
            Native.ExceptionInformation info;
            Native.StGetLastError(out info);

            return String.Format("Simutrace Exception <{0}>: {1}",
                info.site, info.message);
        }
    }

    public delegate void LogPrinter(string message);

    public class Replay : IDisposable
    {
        private unsafe delegate void StreamApplyMethod(IntPtr entry);

        private class Stream : IDisposable
        {
            private bool _disposed = false;

            private uint _session;

            private IntPtr _handle;
            private uint _entrySize;

            public StreamApplyMethod ApplyMethod { get; private set; }

            public uint Id { get; private set; }

            public string Name { get; private set; }
            public ulong Size { get; private set; }
            public ulong CompressedSize { get; private set; }
            public ulong NumEntries { get; private set; }

            public Stream(uint session, string name, StreamApplyMethod apply,
                Guid guid)
            {
                if (String.IsNullOrWhiteSpace(name)) {
                    throw new ArgumentException("name");
                }

                Name = name;
                _session = session;
                ApplyMethod = apply;

                Guid typeGuid;
                if (!_searchStream(out typeGuid)) {
                    Id = Native.INVALID_STREAM_ID;
                    _handle = IntPtr.Zero;
                } else {
                    if ((!guid.Equals(Guid.Empty)) && (!guid.Equals(typeGuid))) {
                        throw new ReplayException(String.Format(
                            "Expected type {0} for stream '{1}', but found {2}.",
                            guid, name, typeGuid));
                    }
                }
            }

            public Stream(uint session, string name, StreamApplyMethod apply) :
                this(session, name, apply, Guid.Empty) { }

            public Stream(uint session, string name) :
                this(session, name, null, Guid.Empty) { }

            public Stream(uint session, uint id)
            {
                _session = session;
                Id = id;

                _handle = IntPtr.Zero;
            }

            ~Stream()
            {
                Dispose(false);
            }

            public void Dispose()
            {
                Dispose(true);
                GC.SuppressFinalize(this);
            }

            protected virtual void Dispose(bool disposing)
            {
                if (_disposed) {
                    return;
                }

                Close();

                _disposed = true;
            }

            private bool _searchStream(out Guid type)
            {
                Native.StreamQueryInformation info;
                uint id = NativeX.StXStreamFindByName(_session, Name, out info);
                if (id == Native.INVALID_STREAM_ID) {
                    type = Guid.Empty;
                    return false;
                }

                Id = id;
                Size = info.stats.rawEntryCount *
                    Native.GetEntrySize(ref info.descriptor.type);

                CompressedSize = info.stats.compressedSize;
                NumEntries = info.stats.entryCount;
                type = info.descriptor.type.id;

                _entrySize = info.descriptor.type.entrySize;
                return true;
            }

            public void Open()
            {
                if ((_handle != IntPtr.Zero) ||
                    (Id == Native.INVALID_STREAM_ID)) {
                    throw new InvalidOperationException();
                }

                _handle = Native.StStreamOpen(_session, Id,
                    Native.QueryIndexType.QSequenceNumber, 0,
                    Native.StreamAccessFlags.SafSequentialScan);
                if (_handle == IntPtr.Zero) {
                    throw new SimutraceException();
                }
            }

            public void Close()
            {
                if (_handle != IntPtr.Zero) {
                    Native.StStreamClose(_handle);
                    _handle = IntPtr.Zero;
                }
            }

            public bool IsValid
            {
                get { return Id != Native.INVALID_STREAM_ID; }
            }

            public IntPtr GetNextEntry()
            {
                if ((_handle == IntPtr.Zero) ||
                    (Native.IsVariableEntrySize(_entrySize))) {
                    return IntPtr.Zero;
                }

                return Native.StGetNextEntry(ref _handle);
            }

            public IntPtr ReadVariableData(ulong reference, IntPtr buffer)
            {
                if ((_handle == IntPtr.Zero) ||
                    (!Native.IsVariableEntrySize(_entrySize))) {
                    throw new InvalidOperationException();
                }

                return Native.StReadVariableData(ref _handle, reference, buffer);
            }
        }

        private class StreamCollection<TKey> : IEnumerable<Stream>, IDisposable
            where TKey : struct
        {
            private bool _disposed = false;

            private Stream[] _streams;

            public StreamCollection()
            {
                if (!typeof(TKey).IsEnum) {
                    throw new ApplicationException("Type is not an enum.");
                }

                var values = Enum.GetValues(typeof(TKey));
                int size = Convert.ToInt32((values.OfType<TKey>()).Max()) + 1;

                _streams = new Stream[size];
            }

            ~StreamCollection()
            {
                Dispose(false);
            }

            public void Dispose()
            {
                Dispose(true);
                GC.SuppressFinalize(this);
            }

            protected virtual void Dispose(bool disposing)
            {
                if (_disposed) {
                    return;
                }

                foreach (Stream s in _streams) {
                    if (s != null) {
                        s.Dispose();
                    }
                }

                _disposed = true;
            }

            public IEnumerator<Stream> GetEnumerator()
            {
                return ((IEnumerable<Stream>)_streams).GetEnumerator();
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return GetEnumerator();
            }

            public Stream this[TKey index]
            {
                get { return _streams[Convert.ToInt32(index)]; }
                set { _streams[Convert.ToInt32(index)] = value; }
            }

            public Stream this[int index]
            {
                get { return _streams[index]; }
                set { _streams[index] = value; }
            }

            public int Length
            {
                get { return _streams.Length; }
            }
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct ScreenEntry
        {
            public ulong CycleCount;
            public uint Width;
            public uint Height;
            public ulong Reference;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct Cr3SwitchEntry
        {
            public ulong CycleCount;
            public ulong Cr3;
        }

        private enum ReplayState
        {
            Running,
            Suspend,
            Done
        }

        private enum Streams
        {
            // Streams that are input for the multiplexer first
            Write = 0x00,
            Screen,
            Cr3,

            ScreenData,
            Multiplexer
        }

        private bool _disposed = false;

        private LogPrinter _logger;

        // Connection
        private string _server;
        private string _store;

        private uint _session = Native.INVALID_SESSION_ID;

        // Input streams
        private StreamCollection<Streams> _streams;

        // Replay
        private ReplayState _state;
        private bool _singleStep;
        private Thread _replayer;

        private ulong _index;
        private ulong _cycle;

        private DateTime _startTime;
        private TimeSpan _suspendTime;

        private Bitmap _screen;
        private Object _screenLock;

        private ulong _cr3;

        // Statistics
        private long[] _numWrites;
        private long   _numCr3Switches;

        // Ram Configuration
        private RamBitmap _ramBitmap;
        private RamMap _ram;

        public Replay(LogPrinter logger, string server, string store,
            string writeStream, uint ramSizeInMiB)
        {
            _logger = logger;
            _server = server;
            _store = store;

            _numWrites = new long[4];
            _streams = new StreamCollection<Streams>();

            _screenLock = new Object();
            _state = ReplayState.Suspend;

            // Connect to the server and open the specified store
            _log("Connecting to server '{0}'...", server);

            _session = Native.StSessionCreate(server);
            if (_session == Native.INVALID_SESSION_ID) {
                throw new SimutraceException();
            }

            _log("ok\n");

            try {
                // Open the specified store
                _log("Opening store '{0}'...", store);

                if (!Native.StSessionOpenStore(_session, store)) {
                    throw new SimutraceException();
                }

                _log("ok\n");

                List<uint> ids = new List<uint>();

                // Find the memory write stream
                _log("Using memory write stream '{0}'...", writeStream);

                Guid mtype = new Guid("{6E943CDD-D2DA-4E83-984F-585C47EB0E36}");
                _streams[Streams.Write] = new Stream(_session, writeStream,
                    _applyWrite, mtype);
                if (!_streams[Streams.Write].IsValid) {
                    throw new ReplayException("Could not find memory stream.");
                }

                ids.Add(_streams[Streams.Write].Id);

                _log("ok\n");

                // Find the streams that contain dumps from the screen output
                _log("Searching for screen streams...");

                Stream screen = new Stream(_session, "screen", _applyScreen);
                Stream screenData = new Stream(_session, "screen_data");
                if (!screen.IsValid || !screenData.IsValid) {
                    screen.Dispose();
                    screenData.Dispose();

                    _log("not found. Screen output not available.\n");
                } else {
                    _streams[Streams.Screen] = screen;
                    _streams[Streams.ScreenData] = screenData;

                    ids.Add(screen.Id);

                    _log("ok\n");
                }

                // Find the stream that contains cr3 change information
                _log("Searching for page directory set stream...");

                Stream cr3 = new Stream(_session, "cpu_set_pagedirectory", _applyCr3);
                if (!cr3.IsValid) {
                    cr3.Dispose();

                    _log("not found\n");
                } else {
                    _streams[Streams.Cr3] = cr3;

                    ids.Add(cr3.Id);

                    _log("ok\n");
                }

                // Create multiplexer
                _streams[Streams.Multiplexer] = new Stream(_session,
                    NativeX.StXMultiplexerCreate(_session, "mult",
                        NativeX.MultiplexingRule.MxrCylceCount,
                        NativeX.MultiplexerFlags.MxfIndirect, ids.ToArray()));
                if (!_streams[Streams.Multiplexer].IsValid) {
                    throw new SimutraceException();
                }

                _streams[Streams.Multiplexer].Open();

                // Allocate a buffer that will hold the guest's RAM and a
                // ram bitmap for visualization
                _log("Configuring ram size for replay {0} MiB...", ramSizeInMiB);

                _ram = new RamMap((ulong)ramSizeInMiB << 20, true);
                _ramBitmap = new RamBitmap(_ram);

                _log("ok\n");

                // Create the thread that will perform the replay
                _log("Creating replay thread...");

                _replayer = new Thread(_replayThreadMain);

                _log("ok\n");

            } catch (Exception) {
                _log("failed\n");
                _finalize();

                throw;
            }
        }

        ~Replay()
        {
            Dispose(false);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (_disposed) {
                return;
            }

            if (disposing) {
                Stop();
                if ((_replayer != null) && (_replayer.IsAlive)) {
                    _replayer.Join();
                }
            }

            _finalize();

            _disposed = true;
        }

        private void _finalize()
        {
            if (_streams != null) {
                _streams.Dispose();
            }

            if (_ramBitmap != null) {
                _ramBitmap.Dispose();
            }

            if (_ram != null) {
                _ram.Dispose();
            }

            if (_screen != null) {
                _screen.Dispose();
            }

            Native.StSessionClose(_session);
            _session = Native.INVALID_SESSION_ID;
        }

        private void _log(string format, params object[] args)
        {
            if (_logger != null) {
                _logger(String.Format(format, args));
            }
        }

        private unsafe void _applyWrite(IntPtr entry)
        {
            var access = (Native.DataMemoryAccess64*)entry;

            uint size = ((access->metadata.flags & 0x0001) != 0) ? 3 :
                    access->data.size;

            _ram.ApplyMemoryAccess(_cycle, access->address,
                access->data.data64, size, false);

            _numWrites[size]++;
        }

        private unsafe void _applyScreen(IntPtr entry)
        {
            var e = (ScreenEntry*)entry;

            IntPtr size = (IntPtr)(e->Width * e->Height * 4);
            IntPtr buffer = Marshal.AllocHGlobal(size);
            try {
                IntPtr r =_streams[Streams.ScreenData].ReadVariableData(
                    e->Reference, buffer);
                if (r != size) {
                    throw new SimutraceException();
                }

                int w = (int)e->Width;
                int h = (int)e->Height;

                lock (_screenLock) {
                    if (_screen != null) {
                        _screen.Dispose();
                        _screen = null;
                    }

                    _screen = new Bitmap(w, h, PixelFormat.Format32bppRgb);
                    BitmapData bd = _screen.LockBits(new Rectangle(0, 0, w, h),
                        ImageLockMode.ReadWrite, PixelFormat.Format32bppRgb);

                    uint* src = (uint*)buffer;
                    uint* dst = (uint*)bd.Scan0;
                    uint* end = dst + w * h;

                    while (dst < end) {
                        *dst++ = *src++;
                    }

                    _screen.UnlockBits(bd);
                }
            } finally {
                Marshal.FreeHGlobal(buffer);
            }
        }

        private unsafe void _applyCr3(IntPtr entry)
        {
            var e = (Cr3SwitchEntry*)entry;

            _cr3 = e->Cr3;

            _numCr3Switches++;
        }

        private unsafe void _doReplay()
        {
            _log("Starting replay...\n");

            _startTime = DateTime.Now;
            Stream multiplexer = _streams[Streams.Multiplexer];

            if (_streams[Streams.ScreenData] != null) {
                _streams[Streams.ScreenData].Open();
            }

            int numStreams = _streams.Length;
            while (_state != ReplayState.Done) {

                // Replaying ---------------------------------
                while (_state == ReplayState.Running) {

                    NativeX.MultiplexerEntry* entry =
                        (NativeX.MultiplexerEntry*)multiplexer.GetNextEntry();

                    if (entry == null) {
                        _state = ReplayState.Done;
                        break;
                    }

                    _cycle = entry->cycleCount;
                    _index++;

                    int idx = (int)entry->streamIdx;
                    _streams[idx].ApplyMethod(entry->entry);

                    if (_singleStep) {
                        _state = ReplayState.Suspend;
                        _singleStep = false;
                    }
                }

                if (_state == ReplayState.Done) {
                    break;
                }

                // Suspend/Resume ---------------------------------
                DateTime suspendTime = DateTime.Now;

                _log("Suspending replay at cycle {0}.\n", _cycle);

                while (_state == ReplayState.Suspend) {
                    Thread.Sleep(100);

                    DateTime now = DateTime.Now;
                    _suspendTime += now - suspendTime;
                    suspendTime = now;
                }

                if (_singleStep) {
                    _log("Single step.\n");
                } else {
                    _log("Resuming replay...\n");
                }
            }
        }

        private void _replayThreadMain(object obj)
        {
            Replay replay = (Replay)obj;

            if (!Native.StSessionOpen(replay._session)) {
                replay._log("Failed to open session.");
            }

            try {

                replay._doReplay();

            } catch (Exception e) {
                if (e is SimutraceException || e is ReplayException) {
                    replay._log(String.Format(
                        "Replay failed at <index: {0}, cycle: {1}>: {2}",
                        replay._index, replay._cycle, e.Message));
                } else {
                    throw;
                }
            } finally {
                Native.StSessionClose(replay._session);
            }
        }

        private void _start(bool singleStep)
        {
            if (_state == ReplayState.Done) {
                throw new InvalidOperationException();
            }

            _state = ReplayState.Running;
            _singleStep = singleStep;

            if (!_replayer.IsAlive) {
                _replayer.Start(this);
            }
        }

        public void Start()
        {
            _start(false);
        }

        public void SingleStep()
        {
            _start(true);
        }

        public void Stop()
        {
            _state = ReplayState.Done;
        }

        public void Suspend()
        {
            if ((_state == ReplayState.Done) || (!_replayer.IsAlive)) {
                throw new InvalidOperationException();
            }

            _state = ReplayState.Suspend;
        }

        public RamBitmap RamBitmap
        {
            get { return _ramBitmap; }
        }

        public string Server
        {
            get { return _server; }
        }

        public string Store
        {
            get { return _store; }
        }

        public string WriteStream
        {
            get { return _streams[Streams.Write].Name; }
        }

        public ulong WriteStreamNumEntries
        {
            get { return _streams[Streams.Write].NumEntries; }
        }

        public ulong WriteStreamSizeCompressed
        {
            get { return _streams[Streams.Write].CompressedSize; }
        }

        public ulong Cycle
        {
            get { return _cycle; }
        }

        public ulong Index
        {
            get { return _index; }
        }

        public long[] NumWrites
        {
            get { return _numWrites; }
        }

        public long NumCr3Switches
        {
            get { return _numCr3Switches; }
        }

        public TimeSpan ReplayTime
        {
            get { return DateTime.Now - _startTime - _suspendTime; }
        }

        public bool IsDone
        {
            get { return (_state == ReplayState.Done); }
        }

        public Bitmap Screen
        {
            get {
                lock (_screenLock) {
                    if (_screen == null) {
                        return null;
                    }

                    return _screen.Clone(new Rectangle(0, 0,
                        _screen.Width, _screen.Height),
                        PixelFormat.Format32bppRgb);
                }
            }
        }
    }
}
