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

#include "FileBackedMemorySegment.h"

#include "Exceptions.h"
#include "MemoryHelpers.h"
#include "Utils.h"

namespace SimuTrace
{

    FileBackedMemorySegment::FileBackedMemorySegment(const std::string& fileName,
                                                     bool writeable) :
        MemorySegment(writeable),
        _name(fileName),
    #ifdef WIN32
        _fileMapping(),
    #else
    #endif
        _file()
    {
        _initFile(writeable);

    #ifdef WIN32
        _initFileMapping();
    #else
    #endif
    }

    FileBackedMemorySegment::FileBackedMemorySegment(
        const FileBackedMemorySegment& instance) :
        MemorySegment(instance),
        _name(instance._name),
    #ifdef WIN32
        _fileMapping(instance._fileMapping),
    #else
    #endif
        _file(instance._file)
    {

    }

    FileBackedMemorySegment::FileBackedMemorySegment(Handle fileMapping,
                                                     bool writeable,
                                                     size_t size,
                                                     const std::string* name) :
        MemorySegment(writeable, size),
        _name((name != nullptr) ? name->c_str() : ""),
    #ifdef WIN32
        _fileMapping(),
    #else
    #endif
        _file()
    {
    #ifdef WIN32
        if (fileMapping == INVALID_HANDLE_VALUE) {
            ThrowOn(_name.empty(), ArgumentException);

            _initFileMapping();
        } else {
            _fileMapping = fileMapping;
        }
    #else
        _file = fileMapping;
    #endif
    }

    FileBackedMemorySegment::~FileBackedMemorySegment()
    {
        _finalize();
    }

    void FileBackedMemorySegment::_initFile(bool writeable)
    {
        // Open the specified file
    #ifdef WIN32
        SECURITY_ATTRIBUTES attr;
        attr.nLength              = sizeof(SECURITY_ATTRIBUTES);
        attr.bInheritHandle       = TRUE;
        attr.lpSecurityDescriptor = nullptr;

        unsigned int fileAccess = (writeable) ?
            GENERIC_WRITE | GENERIC_READ : GENERIC_READ;

        SafeHandle file = ::CreateFileA(_name.c_str(), fileAccess,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        &attr, OPEN_EXISTING, 0, NULL);
    #else
        unsigned int fileAccess = (writeable) ?
            O_RDWR : O_RDONLY;

        SafeHandle file = ::open(_name.c_str(), fileAccess);
    #endif

        ThrowOn(!file.isValid(), PlatformException);

        _file.swap(file);

        _updateSize();
    }

    void FileBackedMemorySegment::_updateSize()
    {
        assert(_file.isValid());

        // Get the file size
        size_t size;
    #ifdef WIN32
        LARGE_INTEGER fileSize;
        BOOL success = ::GetFileSizeEx(_file, &fileSize);
        ThrowOn(!success, PlatformException);

        size = static_cast<size_t>(fileSize.QuadPart);
        size_t oldSize = this->getSize();
    #else
        struct stat buf;
        int result = ::fstat(_file, &buf);
        ThrowOn(result != 0, PlatformException);

        size = buf.st_size;
    #endif

        _setSize(size);

    #ifdef WIN32
        // On Windows, we have to create a new section object if the size of
        // the backing file has changed.
        if ((size != oldSize) && (_fileMapping.isValid())) {
            _initFileMapping();
        }
    #else
    #endif
    }

#ifdef WIN32
    void FileBackedMemorySegment::_initFileMapping()
    {
        SECURITY_ATTRIBUTES attr;
        attr.nLength              = sizeof(SECURITY_ATTRIBUTES);
        attr.bInheritHandle       = TRUE;
        attr.lpSecurityDescriptor = nullptr;

        LARGE_INTEGER size;
        size.QuadPart = this->getSize();
        unsigned int prot = (isReadOnly()) ? PAGE_READONLY : PAGE_READWRITE;
        const char* name = (_file.isValid()) ? nullptr : _name.c_str();

        assert(!(_file.isValid() && (name != nullptr)));
        assert((name == nullptr) || (!_name.empty()));

        _fileMapping = ::CreateFileMappingA(_file, &attr, prot,
                                            size.HighPart, size.LowPart,
                                            name);

        ThrowOn(!_fileMapping.isValid(), PlatformException);
    }
#else
#endif

    byte* FileBackedMemorySegment::_map(size_t start, size_t size)
    {
        void* buffer;

        // The offset needs to be aligned according to the system's allocation
        // granularity. We therefore map more memory if necessary and add the
        // difference to the buffer address at the end.
        static const uint64_t mask = ~(static_cast<uint64_t>(
            System::getMemoryAllocationGranularity()) - 1);

        size_t mappingOffset   = start & mask;
        size_t mappingSizeDiff = start - mappingOffset;

    #ifdef WIN32
        LARGE_INTEGER offset;
        offset.QuadPart = mappingOffset;

        unsigned int prot = (isReadOnly()) ?
            FILE_MAP_READ : FILE_MAP_READ | FILE_MAP_WRITE;

        buffer = ::MapViewOfFile(_fileMapping, prot, offset.HighPart,
                                 offset.LowPart, size + mappingSizeDiff);
        if (buffer == nullptr) {
    #else
        unsigned int prot = (isReadOnly()) ?
            PROT_READ : PROT_READ | PROT_WRITE;

        buffer = ::mmap(0, size + mappingSizeDiff, prot, MAP_SHARED, _file,
                        mappingOffset);
        if (buffer == (char*)-1) {
    #endif
            Throw(PlatformException);
        }

        // Account for the alignment
        return reinterpret_cast<byte*>(buffer) + mappingSizeDiff;
    }

    void FileBackedMemorySegment::_unmap(byte* buffer)
    {
        static const uint64_t mask = ~(static_cast<uint64_t>(
            System::getMemoryAllocationGranularity()) - 1);

        void* alignedBuffer = reinterpret_cast<void*>(
            reinterpret_cast<uint64_t>(buffer) & mask);

    #ifdef WIN32
        if (!::UnmapViewOfFile(alignedBuffer)) {
    #else
        if (::munmap(alignedBuffer, getMappedSize()) != 0) {
    #endif
            Throw(PlatformException);
        }
    }

    void FileBackedMemorySegment::flush()
    {
        ThrowOn((!_file.isValid()) || (isReadOnly()),
                InvalidOperationException);

    #ifdef WIN32
        if (!::FlushFileBuffers(_file)) {
    #else
        if (::fsync(_file) != 0) {
    #endif
            Throw(PlatformException);
        }
    }

    void FileBackedMemorySegment::updateSize()
    {
        ThrowOn(!_file.isValid(), InvalidOperationException);
        ThrowOn(isMapped(), InvalidOperationException);

        _updateSize();
    }

    const std::string& FileBackedMemorySegment::getName() const
    {
        return _name;
    }

    const SafeHandle& FileBackedMemorySegment::getHandle() const
    {
    #ifdef WIN32
        return _fileMapping;
    #else
        return _file;
    #endif
    }

}
