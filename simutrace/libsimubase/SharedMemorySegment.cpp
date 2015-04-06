/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
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
#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "SharedMemorySegment.h"

#include "Exceptions.h"

namespace SimuTrace
{

    SharedMemorySegment::SharedMemorySegment(const std::string& name,
                                             bool writeable, size_t size) :
        FileBackedMemorySegment(_getSharedMemoryHandle(name, writeable, size),
                                writeable, size, &name)
    {

    }

    SharedMemorySegment::SharedMemorySegment(Handle shmHandle, bool writeable,
                                             size_t size) :
        FileBackedMemorySegment(shmHandle, writeable, size, nullptr)
    {

    }

    SharedMemorySegment::SharedMemorySegment(
        const SharedMemorySegment& instance) :
        FileBackedMemorySegment(instance)
    {

    }

    SharedMemorySegment::~SharedMemorySegment()
    {

    }

    Handle SharedMemorySegment::_getSharedMemoryHandle(const std::string& name,
                                                       bool writeable,
                                                       size_t size)
    {
    #if defined(_WIN32)
        return INVALID_HANDLE_VALUE;
    #else
        int protection = (writeable) ? O_RDWR : O_RDONLY;

        Handle file = ::shm_open(name.c_str(), protection | O_CREAT, S_IRWXU | S_IRWXG);
        ThrowOn(file == INVALID_HANDLE_VALUE, PlatformException);

        if (errno != EEXIST) {
            if (::ftruncate(file, size) != 0) {
                int error = System::getLastErrorCode();
                ::close(file);

                Throw(PlatformException, error);
            }

            // Remove file from /dev/shm to avoid stale shared memory on exit
            ::shm_unlink(name.c_str());
        }

        return file;
    #endif
    }

}
