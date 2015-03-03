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

#include "File.h"

#include "Interlocked.h"
#include "Exceptions.h"
#include "Utils.h"

namespace SimuTrace
{

    File::File(const std::string& fileName, CreateMode createMode,
               AccessMode accessMode) :
        _fileName(),
        _accessMode(accessMode),
        _curWritePos(0),
        _curReadPos(0),
        _file()
    {
        _open(fileName, createMode, accessMode);
    }

    File::~File()
    {

    }

    void File::_open(const std::string& fileName, CreateMode createMode,
                     AccessMode accessMode)
    {
        close();

        SafeHandle handle;

    #ifdef WIN32
        SECURITY_ATTRIBUTES attr  = {0};
        attr.nLength              = sizeof(SECURITY_ATTRIBUTES);
        attr.bInheritHandle       = TRUE;
        attr.lpSecurityDescriptor = nullptr;

        handle = ::CreateFileA(fileName.c_str(), accessMode,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               &attr, createMode,
                               FILE_ATTRIBUTE_ARCHIVE |
                               FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    #else
        handle = ::open(fileName.c_str(), accessMode | createMode,
                        S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP);
    #endif

        ThrowOn(!handle.isValid(), PlatformException);

        _file.swap(handle);
        _fileName    = fileName;
        _accessMode  = accessMode;
        _curReadPos  = 0;
        _curWritePos = getSize();
    }

    void File::close()
    {
        _file.close();
    }

    size_t File::read(void* buffer, size_t size, FileOffset offset)
    {
        ThrowOn(!_file.isValid(), InvalidOperationException);
        ThrowOnNull(buffer, ArgumentNullException);
        ThrowOn(size == 0, ArgumentException);

    #ifdef WIN32
        DWORD bytesRead;
        OVERLAPPED _overlappedRead = {0};

        LARGE_INTEGER off;
        off.QuadPart               = offset;
        _overlappedRead.Offset     = off.LowPart;
        _overlappedRead.OffsetHigh = off.HighPart;

        if (!::ReadFile(_file, buffer, (DWORD)size, &bytesRead,
                        &_overlappedRead)) {
            Throw(PlatformException);
        }

        return bytesRead;
    #else
        int result = ::pread(_file, buffer, size, offset);
        ThrowOn(result <= -1, PlatformException);

        return result;
    #endif
    }

    size_t File::read(void* buffer, size_t size)
    {
        // Returns the file offset before the seek.
        FileOffset offset = Interlocked::interlockedAdd(&_curReadPos, size);
        return read(buffer, size, offset);
    }

    size_t File::write(const void* buffer, size_t size, FileOffset offset)
    {
        ThrowOn(!_file.isValid(), InvalidOperationException);
        ThrowOnNull(buffer, ArgumentNullException);
        ThrowOn(size == 0, ArgumentException);

    #ifdef WIN32
        DWORD result;
        LARGE_INTEGER off;

        OVERLAPPED _overlappedWrite = {0};
        off.QuadPart                = offset;
        _overlappedWrite.Offset     = off.LowPart;
        _overlappedWrite.OffsetHigh = off.HighPart;

        if (!::WriteFile(_file, buffer, static_cast<DWORD>(size),
                         &result, &_overlappedWrite)) {
            Throw(PlatformException);
        }
    #else
        int result = ::pwrite(_file, buffer, size, offset);
        ThrowOn(result == 0, PlatformException);
    #endif

        ThrowOn(result != size, Exception, "Failed to write entire buffer.");

        return size;
    }

    size_t File::write(const void* buffer, size_t size)
    {
        FileOffset offset = reserveSpace(size);
        return write(buffer, size, offset);
    }

    void File::flush()
    {
        ThrowOn(!_file.isValid(), InvalidOperationException);

    #ifdef WIN32
        if (!::FlushFileBuffers(_file)) {
            Throw(PlatformException);
        }
    #else
        if (::fsync(_file) != 0) {
            Throw(PlatformException);
        }
    #endif
    }

    void File::truncate()
    {
        ThrowOn(!_file.isValid(), InvalidOperationException);

    #ifdef WIN32
        ::SetFilePointer(_file, 0, nullptr, FILE_BEGIN);
        if (!::SetEndOfFile(_file)) {
            Throw(PlatformException);
        }
    #else
        if (::ftruncate(_file, 0) != 0) {
            Throw(PlatformException);
        }
    #endif
    }

    size_t File::getSize() const
    {
        ThrowOn(!_file.isValid(), InvalidOperationException);

        size_t retVal;
    #ifdef WIN32
        LARGE_INTEGER result;
        if (!::GetFileSizeEx(_file, &result)) {
            Throw(PlatformException);
        }
        retVal = static_cast<size_t>(result.QuadPart);
    #else
        struct stat result;
        if (::fstat(_file, &result) != 0) {
            Throw(PlatformException);
        }
        retVal = result.st_size;
    #endif

        return retVal;
    }

    File::AccessMode File::getAccessMode() const
    {
        return _accessMode;
    }

    const std::string& File::getName() const
    {
        return _fileName;
    }

    FileOffset File::reserveSpace(size_t size)
    {
        return Interlocked::interlockedAdd(&_curWritePos, size);
    }

    FileOffset File::commitSpace(size_t size)
    {
        FileOffset startOffset;
        char put = 0;

        startOffset = reserveSpace(size);
        write(&put, sizeof(char), startOffset + size - sizeof(char));

        return startOffset;
    }

    bool File::isOpen() const
    {
        return (_file.isValid()) ? true : false;
    }

    bool File::exists(const std::string& path)
    {
        try {
            // Test if a file exists by trying to open it.
            File f(path, CreateMode::OpenExisting, AccessMode::ReadOnly);

            return true;
        } catch (...) {
            return false;
        }
    }

    void File::remove(const std::string& path)
    {
        int result = ::remove(path.c_str());
        ThrowOn(result != 0, PlatformException);
    }
}
