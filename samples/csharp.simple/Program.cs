using System;
using System.Runtime.InteropServices;
using SimuTrace;

namespace simple
{
    class Program
    {
        // This will be our custom trace entry with a time stamp (only 48 bits
        // used) and a data field.
        [StructLayout(LayoutKind.Sequential, Pack=1, Size=16)]
        public struct MyEntry
        {
            public ulong CycleCount;
            public ulong Data;
        }

        static void Main(string[] args)
        {
            const int max = 0x400000 * 2 + 0x1000;

            uint session  = Native.INVALID_SESSION_ID;
            uint stream   = Native.INVALID_STREAM_ID;
            IntPtr handle = IntPtr.Zero;

            try {
                Console.WriteLine("[Sample] Connecting to server (write)...");

                // Write data -------------------------------------------------

                // Create a new session and a new store
                session = Native.StSessionCreate("local:/tmp/.simutrace");
                if (session == Native.INVALID_SESSION_ID) {
                    throw new Exception();
                }

                bool success = Native.StSessionCreateStore(session,
                    "simtrace:mystore.sim", true);
                if (!success) {
                    throw new Exception();
                }

                // Register a stream into which we can feed our data
                Native.StreamDescriptor desc;
                success = Native.StMakeStreamDescriptor("My Stream", 16, true,
                    out desc);
                if (!success) {
                    throw new Exception();
                }

                stream = Native.StStreamRegister(session, ref desc);
                if (stream == Native.INVALID_STREAM_ID) {
                    throw new Exception();
                }

                Console.WriteLine("[Sample] Writing data...");

                // Create a write handle and start writing some entries.
                handle = Native.StStreamAppend(session, stream);
                for (int i = 0; i < max; i++) {
                    IntPtr eptr = Native.StGetNextEntry(ref handle);

                    // eptr now contains the address to the current entry in
                    // the unmanaged stream buffer supplied by Simutrace.
                    unsafe {
                        MyEntry* entry = (MyEntry*)eptr.ToPointer();

                        entry->CycleCount = (ulong)i;
                        entry->Data = (ulong)(max - i);
                    }

                    Native.StSubmitEntry(handle);
                }

                Native.StStreamClose(handle);
                Native.StSessionClose(session);
                handle = IntPtr.Zero;

                // We can now read the data back in. Note that we closed the
                // session because for now, it is rather complex to wait for
                // the server finishing processing. Reopening the session (if
                // it is the last one) guarantees that the data will be available.

                Console.WriteLine("[Sample] Connecting to server (read)...");

                // Read data --------------------------------------------------

                // Create a new session and a new store
                session = Native.StSessionCreate("local:/tmp/.simutrace");
                if (session == Native.INVALID_SESSION_ID) {
                    throw new Exception();
                }

                success = Native.StSessionOpenStore(session,
                    "simtrace:mystore.sim");
                if (!success) {
                    throw new Exception();
                }


                Console.WriteLine("[Sample] Querying stream...");

                // Print some information about our stream
                Native.StreamQueryInformation query;
                success = Native.StStreamQuery(session, stream, out query);
                if (!success) {
                    throw new Exception();
                }

                ulong size = (query.stats.entryCount *
                    Native.GetEntrySize(ref query.descriptor.type)) /
                    (1024 * 1024);

                string s;
                s = String.Format("Name: '{0}'  " +
                                  "Type: {1}  " +
                                  "EntryCount: {2}  " +
                                  "Size: {3} MiB  " +
                                  "Compressed Size: {4} MiB",
                                  query.descriptor.name,
                                  query.descriptor.type.name,
                                  query.stats.entryCount,
                                  size,
                                  query.stats.compressedSize / (1024 * 1024));

                Console.WriteLine(s);

                Console.WriteLine("[Sample] Verifying data...");

                // Open the stream for read access and validate entries.
                handle = Native.StStreamOpen(session, stream,
                    Native.QueryIndexType.QCycleCount, 0,
                    Native.StreamAccessFlags.SafSequentialScan);

                for (int i = 0; i < max; i++) {
                    IntPtr eptr = Native.StGetNextEntry(ref handle);
                    unsafe {
                        MyEntry* entry = (MyEntry*)eptr.ToPointer();

                        if ((entry->CycleCount != (ulong)i) ||
                            (entry->Data != (ulong)(max - i))) {

                            // This should never happen!
                            Console.WriteLine("[Sample] Broken entry found!");
                            return;
                        }
                    }
                }

            } catch (Exception) {
                Native.ExceptionInformation info;
                Native.StGetLastError(out info);

                Console.WriteLine("[Sample] Error: " + info.message);
            } finally {
                Console.WriteLine("[Sample] Closing session...");

                Native.StStreamClose(handle);
                Native.StSessionClose(session);
            }

        }
    }
}
