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
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"
#include "SimuBaseLibraries.h"

#include "Exceptions.h"
#include "Environment.h"
#include "Logging.h"

using namespace libconfig;

namespace SimuTrace {
namespace Configuration
{

    const char* settingTypeToString(Setting::Type type);
    std::string settingToString(Setting& setting, uint32_t maxElements = 10);

    template<typename T>
    inline bool tryGet(const char* path, T& out)
    {
        Config* config = Environment::getConfig();
        if (config == nullptr) {
            return false;
        }

        return config->lookupValue(path, out);
    }

    template<typename T>
    inline bool tryGet(const std::string& path, T& out)
    {
        return tryGet<T>(path.c_str(), out);
    }


    template<typename T>
    inline T get(const char* path)
    {
        Config* config = Environment::getConfig();
        assert(config != nullptr);

        return config->lookup(path);
    }

    template<>
    inline const char* get<const char*>(const char* path)
    {
        Config* config = Environment::getConfig();
        assert(config != nullptr);

        return config->lookup(path).c_str();
    }

    template<>
    inline std::string get<std::string>(const char* path)
    {
        return std::string(get<const char*>(path));
    }

    template<typename T>
    inline T get(const std::string& path)
    {
        return get<T>(path.c_str());
    }


    template<typename T>
    inline void get(const char* path, T& out)
    {
        Config* config = Environment::getConfig();
        assert(config != nullptr);

        out = config->lookup(path);
    }

    template<>
    inline void get<const char*>(const char* path, const char*& out)
    {
        out = get<const char*>(path);
    }

    template<>
    inline void get<std::string>(const char* path, std::string& out)
    {
        out = get<std::string>(path);
    }

    template<typename T>
    inline void get(const std::string& path, T& out)
    {
        get<T>(path.c_str(), out);
    }

    // Forward declarations for type map. With this map, we can translate a
    // native language type to a libconfig setting type.
    template<typename T> struct _typeMap;
    template<> struct _typeMap<std::string>;
    template<> struct _typeMap<int>;
    template<> struct _typeMap<long long>;
    template<> struct _typeMap<bool>;

    template<typename T>
    inline bool trySet(const char* path, T& in)
    {
        Config* config = Environment::getConfig();
        if ((config == nullptr) || (!config->exists(path))) {
            return false;
        }

        Setting& setting = config->lookup(path);
        setting = in;
    }

    template<typename T>
    inline bool trySet(const std::string& path, T& in)
    {
        return trySet(path.c_str(), in);
    }

    template<typename T>
    inline void set(const char* path, T& in, bool force = false)
    {
        Config* config = Environment::getConfig();
        assert(config != nullptr);

        Setting* setting = nullptr;
        Setting::Type type = _typeMap<T>::type;
        if (config->exists(path)) {
            setting = &config->lookup(path);

            if (setting->getType() != type) {
                // The type of the setting in the current configuration does
                // not match the supplied type. Usually, that means the caller
                // has chosen the wrong type.

                // Only throw an exception, when we do not force the
                // configuration of the value.
                ThrowOn(!force, ConfigurationException, stringFormat(
                        "Failed to update setting '%s' to value '%s'. "
                        "Expected type is '%s', but found '%s'. "
                        "Keeping value '%s'.",
                        path,
                        valueToString(in).c_str(),
                        settingTypeToString(setting->getType()),
                        settingTypeToString(type),
                        settingToString(*setting).c_str()));

                LogWarn("Forcing setting '%s' to value '%s' of type '%s'. "
                        "Current value is '%s' of type '%s'.",
                        path,
                        valueToString(in).c_str(),
                        settingTypeToString(type),
                        settingToString(*setting).c_str(),
                        settingTypeToString(setting->getType()));

                Setting* parent = (setting->isRoot()) ?
                    setting : &setting->getParent();

                parent->remove(setting->getIndex());

                setting = nullptr;
            }
        }

        if (setting == nullptr) {
            // We need to iteratively create each group. Libconfig
            // does not support adding settings based on path name.

            Setting* parent = &config->getRoot();
            std::string s_path = "";
            std::string::size_type pos;
            std::string option = std::string(path);
            while ((pos = option.find('.')) != std::string::npos) {
                std::string token = option.substr(0, pos);
                s_path += token;

                if (parent->exists(token.c_str())) {
                    Setting* s = &config->lookup(s_path);
                    if (!s->isGroup()) {
                        ThrowOn(!force, ConfigurationException, stringFormat(
                                "Failed to update setting '%s' to value '%s'. "
                                "Path already used for setting '%s'.",
                                path,
                                valueToString(in).c_str(),
                                s_path.c_str()));

                        LogWarn("Overwriting setting '%s' to create "
                                "configuration group for setting '%s'.",
                                s_path.c_str(),
                                path);

                        // s cannot be the root, because it is not a group
                        assert(!s->isRoot());

                        Setting& p = s->getParent();
                        p.remove(s->getIndex());

                        s = &p.add(token.c_str(), Setting::TypeGroup);
                    }

                    parent = s;
                } else {
                    parent = &parent->add(token.c_str(), Setting::TypeGroup);
                }

                assert(parent != nullptr);
                option = option.substr(pos + 1);
                s_path += ".";
            }

            setting = &parent->add(option.c_str(), type);
        }

        // We successfully created the setting's path. Now set the value.
        assert(setting != nullptr);
        *setting = in;

        LogDebug("Setting: %s = %s", path, valueToString(in).c_str());
    }

    template<typename T>
    static inline void set(const std::string& path, T& in, bool force = false)
    {
        set<T>(path.c_str(), in, force);
    }

    struct ConfigurationLock {
        enum Type {
            ClkAlways,
            ClkIfExists
        };

        Type type;
        std::string path;

        ConfigurationLock(Type type, const std::string& path) :
            type(type),
            path(path)
        { }
    };

    typedef std::vector<ConfigurationLock> LockList;

    void apply(Setting& setting, LockList* lockList);

    //
    // Internal Helper Templates
    //

    template<typename T> struct _typeMap {};
    template<> struct _typeMap<std::string> {
        static const Setting::Type type = Setting::Type::TypeString;
    };
    template<> struct _typeMap<int> {
        static const Setting::Type type = Setting::Type::TypeInt;
    };
    template<> struct _typeMap<long long> {
        static const Setting::Type type = Setting::Type::TypeInt64;
    };
    template<> struct _typeMap<bool> {
        static const Setting::Type type = Setting::Type::TypeBoolean;
    };

}
}

#endif