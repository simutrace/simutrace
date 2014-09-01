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

#include "SafeHandle.h"

#include "Exceptions.h"
#include "Utils.h"

namespace SimuTrace
{

    SafeHandle::SafeHandle(Handle handle) :
        _handle(handle)
    {
        
    }

    SafeHandle::SafeHandle(const SafeHandle& instance) :
        _handle(System::duplicateHandle(instance._handle))
    {

    }

    SafeHandle::~SafeHandle()
    {
        close();
    }

    SafeHandle& SafeHandle::operator=(SafeHandle instance)
    {
        swap(instance);

        return *this;
    }

    SafeHandle& SafeHandle::operator=(Handle handle)
    {
        System::closeHandle(_handle);
        _handle = handle;

        return *this;
    }

    SafeHandle::operator Handle() const
    {
        return _handle;
    }

    void swap(SafeHandle& first, SafeHandle& second)
    {
        using std::swap;

        swap(first._handle, second._handle);
    }

    void SafeHandle::swap(SafeHandle& handle)
    {
        using std::swap;

        swap(*this, handle);
    }

    void SafeHandle::swap(Handle& handle)
    {
        using std::swap;

        swap(_handle, handle);
    }

    bool SafeHandle::isValid() const
    {
        // The safe handle also regards NULL as invalid handle value!
        return ((_handle != INVALID_HANDLE_VALUE) && (_handle != 0));
    }

    void SafeHandle::close()
    {
        if (_handle != 0) {
            System::closeHandle(_handle);
        }

        _handle = INVALID_HANDLE_VALUE;
    }

}