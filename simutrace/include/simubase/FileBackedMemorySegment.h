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
#ifndef FILE_BACKED_MEMORY_SEGMENT_H
#define FILE_BACKED_MEMORY_SEGMENT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "MemorySegment.h"
#include "SafeHandle.h"

namespace SimuTrace
{

    class FileBackedMemorySegment :
        public MemorySegment
    {
    private:
        std::string _name;
        SafeHandle _file;
    #ifdef WIN32
        SafeHandle _fileMapping;
    #else
    #endif

        void _initFile(bool writeable);
        void _updateSize();
    #ifdef WIN32
        void _initFileMapping();
    #else
    #endif
    protected:
        FileBackedMemorySegment(Handle fileMapping, bool writeable, size_t size,
                                const std::string* name);

        virtual byte* _map(size_t start, size_t size) override;
        virtual void _unmap(byte* buffer) override;

    public:
        FileBackedMemorySegment(const std::string& fileName, bool writeable);
        FileBackedMemorySegment(const FileBackedMemorySegment& instance);
        virtual ~FileBackedMemorySegment();

        FileBackedMemorySegment& operator=(FileBackedMemorySegment instance) = delete;

        void flush();
        void updateSize();

        const std::string& getName() const;

        const SafeHandle& getHandle() const;
    };
}
#endif