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
#ifndef SIMUPLATFORM_H
#define SIMUPLATFORM_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>

#pragma comment(lib,"Ws2_32.lib")

#include <MSWSock.h>

#include <sys/stat.h>

#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/un.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#include <dirent.h>
#include <fcntl.h>

#include <errno.h>

#include <string.h>
#endif

#include <assert.h>

#ifndef BOOL
#define BOOL unsigned char
#endif

typedef char byte;

#ifndef TRUE
#define FALSE   0
#define TRUE    !FALSE
#endif

#ifdef WIN32
#define _DEBUG_BREAK_ __debugbreak();
#else
#define _DEBUG_BREAK_ assert(FALSE);

#ifndef NDEBUG
#define _DEBUG
#endif

#endif

#include <time.h>

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>

#define KiB *0x400ULL
#define MiB *0x100000ULL
#define GiB *0x40000000ULL

#ifdef WIN32
#define OPT_SHORT_PREFIX "-"
#define OPT_LONG_PREFIX "--"
#else
#define OPT_SHORT_PREFIX "-"
#define OPT_LONG_PREFIX "--"
#endif

/* STL classes */
#ifdef __cplusplus

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include <exception>

#include <set>
#include <unordered_map>
#include <list>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <tuple>
#include <atomic>

#include <limits>
#include <algorithm>

#include <ctime>

#include <memory>

#ifdef WIN32
#define cbegin(vec) (vec).cbegin()
#define cend(vec) (vec).cend()
#else
#define cbegin(vec) (vec).begin()
#define cend(vec) (vec).end()
#endif

#ifdef _MSC_VER
#define __thread _declspec(thread)
#endif

/* Macro to delete copy constructor and assignment operator */
#define DISABLE_COPY(Class) \
    Class(const Class &) = delete;            \
    Class &operator=(const Class &) = delete

#endif /* __cplusplus */

#endif