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
#pragma once
#ifndef FILE_H
#define FILE_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "SafeHandle.h"

namespace SimuTrace
{

    class File
    {
    public:
        enum AccessMode {
        #ifdef WIN32
            ReadOnly  = GENERIC_READ,
            ReadWrite = GENERIC_READ | GENERIC_WRITE
        #else
            ReadOnly  = O_RDONLY,
            ReadWrite = O_RDWR
        #endif
        };

        enum CreateMode {
        #ifdef WIN32
            CreateAlways = CREATE_ALWAYS,
            OpenAlways   = OPEN_ALWAYS,
            OpenExisting = OPEN_EXISTING
        #else
            CreateAlways = O_CREAT | O_TRUNC,
            OpenAlways   = O_CREAT,
            OpenExisting = 0
        #endif
        };

    private:
        DISABLE_COPY(File);

        std::string _fileName;
        AccessMode  _accessMode;

        FileOffset  _curWritePos;
        FileOffset  _curReadPos;

        SafeHandle _file;

        void _open(const std::string& fileName, CreateMode createMode,
                   AccessMode accessMode);
    public:
        ///
        /// Opens the requested file
        ///
        File(const std::string& fileName,
             CreateMode createMode = CreateMode::OpenExisting,
             AccessMode accessMode = AccessMode::ReadWrite);
        ~File();

        ///
        /// Closes the file handle, it is automatically closed when
        /// the object is freed
        ///
        void close();

        ///
        ///
        ///
        size_t read(void* buffer, size_t size, FileOffset offset);
        size_t read(void* buffer, size_t size);

        template<typename T>
        size_t read(T* buffer, FileOffset offset)
        {
            return read(reinterpret_cast<void*>(buffer), sizeof(T), offset);
        }

        template<typename T>
        size_t read(T* buffer)
        {
            return read(reinterpret_cast<void*>(buffer), sizeof(T));
        }

        size_t write(const void* buffer, size_t size, FileOffset offset);
        size_t write(const void* buffer, size_t size);

        template<typename T>
        size_t write(const T* buffer, FileOffset offset)
        {
            return write(reinterpret_cast<const void*>(buffer), sizeof(T),
                         offset);
        }

        template<typename T>
        size_t write(const T* buffer)
        {
            return write(reinterpret_cast<const void*>(buffer), sizeof(T));
        }

        ///
        /// Flushes the buffers associated with the file handle to disk
        ///
        void flush();

        ///
        /// Sets the file size to 0
        ///
        void truncate();

        ///
        /// Returns the current file size
        ///
        size_t getSize() const;

        ///
        /// Returns the filename
        ///
        const std::string& getName() const;

        ///
        /// Returns the access mode used during open
        ///
        AccessMode getAccessMode() const;

        ///
        /// Reserves size bytes of data space
        ///
        FileOffset reserveSpace(size_t size);
        FileOffset commitSpace(size_t size);

        bool isOpen() const;

        static bool exists(const std::string& path);

        static void remove(const std::string& path);
    };

}
#endif