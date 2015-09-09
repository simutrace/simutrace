/*
 * Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus
 *
 *             _____ _                 __
 *            / ___/(_)___ ___  __  __/ /__________ _________
 *            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
 *           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
 *          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/
 *                         http://simutrace.org
 *
 * Simutrace .Net Binding (SimuTrace.Net.Interop) is part of Simutrace.
 *
 * SimuTrace.Net.Interop is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimuTrace.Net.Interop is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SimuTrace.Net.Interop. If not, see <http://www.gnu.org/licenses/>.
 */
using System;
using System.Runtime.InteropServices;

namespace SimuTrace
{
    public static partial class NativeX
    {
        /* SimuTraceX Types */

        public enum MultiplexingRule
        {
            MxrRoundRobin = 0x00,
            MxrCylceCount = 0x01
        }

        [Flags]
        public enum MultiplexerFlags
        {
            MxfNone = 0x00,
            MxfIndirect = 0x01
        }

        // Type Id: {FA258383-9EC8-4E05-8239-9F260DDA8AD2}
        // Type Id: {80C8C1BB-32C9-4283-B95F-BA37C62095CB} - with cycle count
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct MultiplexerEntry
        {
            public ulong cycleCount;
            public uint streamId;
            public uint streamIdx;

            public IntPtr entry;
        }


        /* Helpers */

        /// <summary>
        /// This method searches the list of available streams for a stream
        /// with the given name and returns its id and detailed properties.
        /// </summary>
        /// <param name="session">The id of the session in which to search for
        ///     the stream.</param>
        /// <param name="name">The name of the stream to search for.</param>
        /// <param name="info">Pointer to a StreamQueryInformation structure to
        ///     receive detailed stream information.</param>
        /// <returns>The id of the stream with the given name on success,
        ///     INVALID_STREAM_ID, otherwise. For a more detailed error
        ///     description call StGetLastError().</returns>
        public static uint StXStreamFindByName(uint session, string name,
            out Native.StreamQueryInformation info)
        {
            return NativeMethods.StXStreamFindByName(session, name, out info);
        }


        /// <summary>
        /// This method searches the list of available streams for a stream
        /// with the given name and returns its id and detailed properties.
        /// </summary>
        /// <param name="session">The id of the session in which to search for
        ///     the stream.</param>
        /// <param name="name">The name of the stream to search for.</param>
        /// <returns>The id of the stream with the given name on success,
        ///     INVALID_STREAM_ID, otherwise. For a more detailed error
        ///     description call StGetLastError().</returns>
        public static uint StXStreamFindByName(uint session, string name)
        {
            Native.StreamQueryInformation info;
            return StXStreamFindByName(session, name, out info);
        }


        /// <summary>
        /// Writes a string to the supplied variable data stream
        /// </summary>
        /// <param name="handle">Handle to a variable data stream that should
        ///     store the stream.</param>
        /// <param name="str">The string to save.</param>
        /// <param name="reference">A output variable to receive the reference
        ///     to the string in the stream if successful.</param>
        /// <returns>The length of the string if successful, -1 otherwise.
        ///     </returns>
        public static int StXWriteString(ref IntPtr handle, string str,
            out ulong reference)
        {
            int len;

            GCHandle strh = new GCHandle();
            try {
                strh = GCHandle.Alloc(str, GCHandleType.Pinned);

                len = Native.StWriteVariableData(ref handle,
                    strh.AddrOfPinnedObject(), (IntPtr)str.Length,
                    out reference).ToInt32();
            } finally {
                if (strh.IsAllocated) {
                    strh.Free();
                }
            }

            return len;
        }


        /// <summary>
        /// Reads a string from the supplied variable data stream
        /// </summary>
        /// <param name="handle">Handle to the variable data stream that
        ///     contains the string.</param>
        /// <param name="reference">The reference to the string that was
        ///     returned by StWriteVariableData() during recording.</param>
        /// <param name="str">Output variable for the requested string.</param>
        /// <returns>The length of the requested string, if successful,
        ///     -1 otherwise. For a more detailed error description call
        ///     StGetLastError().</returns>
        public static int StXReadString(ref IntPtr handle, ulong reference,
            out string str)
        {
            str = String.Empty;

            int length = Native.StReadVariableData(ref handle, reference,
                IntPtr.Zero).ToInt32();
            if (length == -1) {
                return -1;
            }

            IntPtr buffer = Marshal.AllocHGlobal(length);
            try {
                int length2 = Native.StReadVariableData(ref handle,
                    reference, buffer).ToInt32();
                if (length2 != length) {
                    return -1;
                }

                str = Marshal.PtrToStringAnsi(buffer, length);
            } finally {
                Marshal.FreeHGlobal(buffer);
            }

            return length;
        }


        /* Stream Multiplexer */

        /// <summary>
        /// A stream multiplexer chooses the next entry from a set of input
        /// streams based on a defined rule. The multiplexer is accesses like a
        /// regular stream.
        /// </summary>
        /// <param name="session">The session for which the multiplexer should
        ///     be created.</param>
        /// <param name="name">A friendly name for the multiplexer.</param>
        /// <param name="rule">The multiplexing rule, which determines the
        ///     next entry.</param>
        /// <param name="flags">A set of flags to customize multiplexing
        ///     behavior.</param>
        /// <param name="inputStreams">array of streams that should be used as
        ///     input for the multiplexer</param>
        /// <returns>The id the dynamic stream representing the created
        ///     multiplexer. Read entries from the multiplexer by reading
        ///     entries from the stream with the returned id.</returns>
        public static uint StXMultiplexerCreate(uint session, string name,
            MultiplexingRule rule, MultiplexerFlags flags, uint[] inputStreams)
        {
            GCHandle ids = new GCHandle();
            try {
                ids = GCHandle.Alloc(inputStreams, GCHandleType.Pinned);
                uint count = (uint)inputStreams.Length;

                return NativeMethods.StXMultiplexerCreate(session, name,
                    rule, flags, ids.AddrOfPinnedObject(), count);
            } finally {
                if (ids.IsAllocated) {
                    ids.Free();
                }
            }
        }

    }
}