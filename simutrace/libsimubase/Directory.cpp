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

#include "Directory.h"

#include "Exceptions.h"
#include "Utils.h"

namespace SimuTrace
{

    Directory::Directory(const std::string& path, const std::string& pattern) :
        _pathname(path),
        _pattern(pattern)
    {

    #if defined(_WIN32)
        _handle = ::FindFirstFileA((_pathname + "*" + _pattern).c_str(), &_data);
        ThrowOn(_handle == INVALID_HANDLE_VALUE, PlatformException);
    #else
        _handle = ::opendir(path.c_str());
        ThrowOnNull(_handle, PlatformException);
    #endif

    }

    Directory::~Directory(void)
    {
    #if defined(_WIN32)
        ::FindClose(_handle);
    #else
        ::closedir(_handle);
    #endif
    }

    bool Directory::exists(const std::string& path)
    {
        struct stat s;
        if (::stat(path.c_str(), &s) == 0) {
            return ((s.st_mode & S_IFDIR) != 0);
        } else {
            if (errno == ENOENT) {
                return false;
            }

            Throw(PlatformException);
        }
    }

    void Directory::create(const std::string& path)
    {
    #if defined(_WIN32)
        BOOL result = CreateDirectoryA(path.c_str(), NULL);
        ThrowOn(!result, PlatformException);
    #else
        int result = ::mkdir(path.c_str(), 0777);
        ThrowOn(result != 0, PlatformException);
    #endif
    }

    std::string Directory::getWorkingDirectory()
    {
    #if defined(_WIN32)
        char buffer[MAX_PATH];
        if (::GetCurrentDirectoryA(MAX_PATH, buffer) == 0) {
            Throw(PlatformException);
        }
    #else
        char buffer[PATH_MAX];
        if (::getcwd(buffer, PATH_MAX) == nullptr) {
            Throw(PlatformException);
        }
    #endif
        return std::string(buffer);
    }

    void Directory::setWorkingDirectory(const std::string& path)
    {
    #if defined(_WIN32)
        if (!::SetCurrentDirectoryA(path.c_str())) {
            Throw(PlatformException);
        }
    #else
        if (::chdir(path.c_str()) != 0) {
            Throw(PlatformException);
        }
    #endif
    }

    std::string Directory::getAbsolutePath(const std::string& path)
    {
    #if defined(_WIN32)
        char buffer[MAX_PATH];
        if(::GetFullPathNameA(path.c_str(), MAX_PATH, buffer, nullptr) == 0) {
            Throw(PlatformException);
        }
    #else
        char buffer[PATH_MAX];
        if (::realpath(path.c_str(), buffer) == nullptr) {
            Throw(PlatformException);
        }
    #endif
        return std::string(buffer);
    }

    std::string Directory::getDirectory(const std::string& path)
    {
        size_t position = path.rfind(_pathDelimiter);
        ThrowOn(position == std::string::npos, ArgumentException);

        return path.substr(0, position + 1);
    }

    std::string Directory::getFileName(const std::string& path)
    {
        size_t position = path.rfind(_pathDelimiter);
        ThrowOn(position == std::string::npos, ArgumentException);

        return path.substr(position + 1);
    }

    std::string Directory::makePath(const std::string* directory,
                                    const std::string* fileName)
    {
        std::string path;

        if ((directory == nullptr) || (directory->empty())) {
            path.assign(getWorkingDirectory());
        } else {
            path.assign(*directory);
        }

        if ((fileName != nullptr) && (!fileName->empty())) {
            path = path + _pathDelimiter + *fileName;
        }

        return path;
    }

    uint32_t Directory::enumerateFiles(std::vector<std::string>& out)
    {
        uint32_t numFiles = 0;
        std::string str;
        size_t pos;
    #if defined(_WIN32)
        do {
            str = std::string(_data.cFileName);
    #else
        _data = ::readdir(_handle);
        while (_data != nullptr) {
            str = std::string(_data->d_name);
    #endif
            // For Windows FindNextFile() has already filtered the results
            // according to the specified pattern. Linux returns ALL files in
            // a directory.

            // TODO: Apply same filtering for Linux. For now, we use the
            // _pattern as a suffix to filter files by ending.
            pos = str.rfind(_pattern);
            if (pos != std::string::npos) {
                str = str.substr(0, pos);
                out.push_back(str);

                numFiles++;
            }
    #if defined(_WIN32)
        } while (::FindNextFileA(_handle, &_data));
    #else
            _data = ::readdir(_handle);
        }
    #endif

        return numFiles;
    }

    const std::string& Directory::getPath() const
    {
        return _pathname;
    }

    const std::string& Directory::getPattern() const
    {
        return _pattern;
    }

}