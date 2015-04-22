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
    public static partial class Native
    {
        static private class NativeMethods
        {
            /* Base API */

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern uint StGetClientVersion();

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern int StGetLastError(
                out ExceptionInformation informationOut);

            /* Session API */

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl,
                CharSet = CharSet.Ansi,
                BestFitMapping=false,
                ThrowOnUnmappableChar=true)]
            public static extern uint StSessionCreate(string server);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StSessionOpen(uint session);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StSessionClose(uint session);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl,
                CharSet = CharSet.Ansi,
                BestFitMapping = false,
                ThrowOnUnmappableChar = true)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StSessionCreateStore(uint session,
                string specifier, [MarshalAs(UnmanagedType.I1)] bool alwaysCreate);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl,
                CharSet = CharSet.Ansi,
                BestFitMapping = false,
                ThrowOnUnmappableChar = true)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StSessionOpenStore(uint session,
                string specifier);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl,
                CharSet = CharSet.Ansi)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StSessionCloseStore(uint session);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl,
                CharSet = CharSet.Ansi,
                BestFitMapping = false,
                ThrowOnUnmappableChar = true)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StSessionSetConfiguration(uint session,
                string configuration);

            /* Stream API */

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl,
                CharSet = CharSet.Ansi,
                BestFitMapping = false,
                ThrowOnUnmappableChar = true)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StMakeStreamDescriptor(string name,
                uint entrySize, [MarshalAs(UnmanagedType.I1)] bool temporalOrder,
                out StreamDescriptor descOut);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl,
                CharSet = CharSet.Ansi,
                BestFitMapping = false,
                ThrowOnUnmappableChar = true)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StMakeStreamDescriptorFromType(
                string name, ref StreamTypeDescriptor type,
                out StreamDescriptor descOut);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            [return: MarshalAs(UnmanagedType.LPStruct)]
            public static extern StreamTypeDescriptor StStreamFindMemoryType(
                ArchitectureSize size, MemoryAccessType accessType,
                MemoryAddressType addressType,
                [MarshalAs(UnmanagedType.I1)] bool hasData);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern uint StStreamRegister(uint session,
                ref StreamDescriptor desc);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern int StStreamEnumerate(uint session,
                IntPtr bufferSize, IntPtr streamIdsOut);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StStreamQuery(uint session, uint stream,
                out StreamQueryInformation informationOut);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr StStreamAppend(uint session,
                uint stream, IntPtr handle);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr StStreamOpen(uint session, uint stream,
                QueryIndexType type, ulong value, StreamAccessFlags flags,
                IntPtr handle);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool StStreamClose(IntPtr handle);

            /* Tracing API */

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr StGetNextEntry(ref IntPtr handle);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr StGetPreviousEntry(ref IntPtr handle);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern void StSubmitEntry(IntPtr handle);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr StWriteVariableData(ref IntPtr handle,
                IntPtr sourceBuffer, IntPtr sourceLength, out ulong referenceOut);

            [DllImport("libsimutrace",
                CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr StReadVariableData(ref IntPtr handle,
                ulong reference, IntPtr destinationBuffer);
        }
    }
}
