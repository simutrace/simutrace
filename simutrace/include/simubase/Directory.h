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
#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

namespace SimuTrace
{

    class Directory
    {
    private:
        DISABLE_COPY(Directory);

        std::string _pathname;
        std::string _pattern;

    #ifdef WIN32
        Handle _handle;
        WIN32_FIND_DATAA _data;
        static const char _pathDelimiter = '\\';
    #else
        DIR* _handle;
        struct dirent *_data;
        static const char _pathDelimiter = '/';
    #endif

    public:
        Directory(const std::string& path, const std::string& pattern);
        ~Directory();

        uint32_t enumerateFiles(std::vector<std::string>& out);

        const std::string& getPath() const;
        const std::string& getPattern() const;

        static bool exists(const std::string& path);

        static std::string getWorkingDirectory();

        static void setWorkingDirectory(const std::string& path);

        static std::string getAbsolutePath(const std::string& path);
        static std::string getDirectory(const std::string& path);
        static std::string getFileName(const std::string& path);

        static std::string makePath(const std::string* directory,
                                    const std::string* fileName);
    };

}

#endif