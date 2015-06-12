using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace csharp.memreplay
{
    public struct FrameMetadata
    {
        public ulong LastWriteAccess;
        public ulong LastReadAccess;
    }

    public class RamMap : IDisposable
    {
        private bool _disposed = false;

        private IntPtr _ram = IntPtr.Zero;

        // Each frame is 4KiB bytes in size, we hold stats in this granularity
        private FrameMetadata[] _frames;
        private ulong _size;

        private ulong _cycle;
        private ulong _index;

        public RamMap(ulong ramSize, bool captureData)
        {
            _size = ramSize;
            _index = 0;

            if (captureData) {
                _ram = Marshal.AllocHGlobal((IntPtr)_size);
            }

            _frames = new FrameMetadata[_size >> 12];
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

            }

            if (_ram != IntPtr.Zero) {
                Marshal.FreeHGlobal(_ram);
                _ram = IntPtr.Zero;
            }

            _disposed = true;
        }

        public unsafe void ApplyMemoryAccess(ulong cycle, ulong address,
            ulong data, uint size, bool read)
        {
            _cycle = cycle;
            _index++;

            if (address + size >= _size) {
                return;
            }

            if ((!read) && (_ram != IntPtr.Zero)) {
                switch (size) {
                    case 3: {
                        ulong* ptr = (ulong*)(&(((byte*)_ram)[address]));
                        *ptr = data;
                        break;
                    }

                    case 2: {
                        uint* ptr = (uint*)(&(((byte*)_ram)[address]));
                        *ptr = (uint)data;
                        break;
                    }

                    case 1: {
                        ushort* ptr = (ushort*)(&(((byte*)_ram)[address]));
                        *ptr = (ushort)data;
                        break;
                    }

                    case 0: {
                        ((byte*)_ram)[address] = (byte)data;
                        break;
                    }

                    default:
                        throw new ReplayException(
                            "Size of memory access exceeds 8 byte.");
                }
            }

            // The access may span multiple sub frames if unaligned
            if (read) {
                _frames[(address +    0) >> 12].LastReadAccess = _index;
                _frames[(address + size) >> 12].LastReadAccess = _index;
            } else {
                _frames[(address + 0)    >> 12].LastWriteAccess = _index;
                _frames[(address + size) >> 12].LastWriteAccess = _index;
            }
        }

        public ulong Size
        {
            get { return _size; }
        }

        public int NumFrames
        {
            get { return (int)(_size >> 12); }
        }

        public FrameMetadata[] Frames
        {
            get { return _frames; }
        }

        public ulong Cycle
        {
            get { return _cycle; }
        }

        public ulong Index
        {
            get { return _index; }
        }
    }
}
