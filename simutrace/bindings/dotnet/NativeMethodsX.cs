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
        static private class NativeMethods
        {
            private const string libsimutraceX = "libsimutraceX";

            /* Helpers */

            [DllImport(libsimutraceX,
                CallingConvention = CallingConvention.Cdecl,
                CharSet = CharSet.Ansi,
                BestFitMapping = false,
                ThrowOnUnmappableChar = true)]
            public static extern uint StXStreamFindByName(uint session,
                string name, out Native.StreamQueryInformation info);


            /* Stream Multiplexer */

            [DllImport(libsimutraceX,
                CallingConvention = CallingConvention.Cdecl,
                CharSet = CharSet.Ansi,
                BestFitMapping = false,
                ThrowOnUnmappableChar = true)]
            public static extern uint StXMultiplexerCreate(uint session,
                string name, MultiplexingRule rule, MultiplexerFlags flags,
                IntPtr inputStreams, uint count);
        }
    }
}
