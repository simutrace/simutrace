/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 *             _____ _                 __
 *            / ___/(_)___ ___  __  __/ /__________ _________ 
 *            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
 *           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
 *          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/ 
 *                         http://simutrace.org
 *
 * Simutrace Base Library (libsimubase) is part of Simutrace.
 *
 * libsimubase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimubase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimubase. If not, see <http://www.gnu.org/licenses/>.
 */
/*! \file */
#pragma once
#ifndef SIMUBASE_H
#define SIMUBASE_H

#include "Version.h"

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"
#include "SimuBaseLibraries.h"

//
// Miscellaneous
//

#include "Utils.h"
#include "Exceptions.h"
#include "IdAllocator.h"
#include "Configuration.h"
#include "Compression.h"
#include "Hash.h"

//
// Time
//

#include "Clock.h"
#include "StopWatch.h"

//
// Threading
//

#include "Interlocked.h"
#include "CriticalSection.h"
#include "LockRegion.h"
#include "ReaderWriterLock.h"
#include "ReaderWriterLockRegion.h"
#include "ConditionVariable.h"
#include "Event.h"
#include "ThreadBase.h"
#include "Thread.h"
#include "WaitContext.h"

//
// Memory Management
//

#include "MemoryHelpers.h"
#include "MemorySegment.h"
#include "PrivateMemorySegment.h"
#include "FileBackedMemorySegment.h"
#include "SharedMemorySegment.h"

//
// Communication
//

#include "Port.h"
#include "ServerPort.h"
#include "ClientPort.h"

//
// I/O
//

#include "File.h"
#include "Directory.h"

//
// Logging
//

#include "Logging.h"
#include "LogEvent.h"
#include "LogPriority.h"
#include "LogCategory.h"

#include "LogAppender.h"
#include "OstreamLogAppender.h"
#include "TerminalLogAppender.h"

#include "LogLayout.h"
#include "SimpleLogLayout.h"
#include "PatternLogLayout.h"

#include "Environment.h"

//
// Profiling
//
#ifdef SIMUTRACE_PROFILING_ENABLE
#include "Profiler.h"
#include "ProfileEntry.h"
#endif

#endif // SIMUBASE_H