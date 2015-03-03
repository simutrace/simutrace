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
#include "SimuBaseLibraries.h"

#include "Configuration.h"

using namespace libconfig;

namespace SimuTrace {
namespace Configuration
{

    const char* settingTypeToString(Setting::Type type)
    {
        static const char* typeNames[] {
            "None",
            "Integer",
            "Integer (64 Bit)",
            "Floating Point",
            "String",
            "Boolean",
            "Group",
            "Array",
            "List"
        };

        assert(type < (sizeof(typeNames) / sizeof(char*)));
        return typeNames[type];
    }

    std::string settingToString(Setting& setting, uint32_t maxElements)
    {
        Setting::Type t = setting.getType();
        switch (t)
        {
            case Setting::Type::TypeInt: {
                return stringFormat("%i", (int)setting);
            }

            case Setting::Type::TypeInt64: {
                return stringFormat("%ll", (long long)setting);
            }

            case Setting::Type::TypeString: {
                return stringFormat("\"%s\"", (const char*)setting);
            }

            case Setting::Type::TypeBoolean: {
                return stringFormat("%i", (bool)setting);
            }

            case Setting::Type::TypeArray:
            case Setting::Type::TypeList: {
                std::ostringstream value;
                value << (t == Setting::Type::TypeArray) ? "[" : "(";

                if (setting.getLength() > 0) {
                    int i = 0;
                    while (true) {
                        value << settingToString(setting[i++]);

                        if (i == setting.getLength()) {
                            break;
                        }

                        if (i == maxElements) {
                            value << ",...";
                            break;
                        } else {
                            value << ",";
                        }
                    }
                } else {
                    value << "empty";
                }

                value << (t == Setting::Type::TypeArray) ? "]" : ")";

                return value.str();
            }

            default: {
                return std::string("<unknown-type>");
            }
        }
    }

    bool _isLocked(Setting& setting, bool exists, LockList* lockList)
    {
        // Compare the path of the current setting with the lock list and
        // prevent changes if the path is found.
        if (lockList != nullptr) {
            std::string path = setting.getPath();

            for (int i = 0; i < lockList->size(); ++i) {
                ConfigurationLock& lock = (*lockList)[i];

                if ((path.compare(lock.path) == 0) &&
                    ((lock.type == ConfigurationLock::Type::ClkAlways) ||
                     ((lock.type == ConfigurationLock::Type::ClkIfExists) && exists))) {

                    return true;
                }
            }
        }

        return false;
    }

    void _apply(Setting& root, Setting& source, LockList* lockList)
    {
        Setting* s = nullptr;

        if (!source.isRoot()) {
            const char* name = source.getName();
            if (name != nullptr) {
                if (root.exists(name)) {
                    s = &root[name];

                    if (_isLocked(*s, true, lockList)) {
                        LogWarn("Ignored updating configuration '%s' to value "
                                "'%s'. Setting locked to value '%s'.",
                                s->getPath().c_str(),
                                settingToString(source).c_str(),
                                settingToString(*s).c_str());

                        return;
                    }

                    if (!s->isGroup()) {
                        root.remove(s->getIndex());
                        s = &root.add(name, source.getType());
                    }
                } else {
                    s = &root.add(name, source.getType());

                    if (_isLocked(*s, false, lockList)) {
                        LogWarn("Ignored creating configuration '%s' with "
                                "value '%s'. Setting locked.",
                                s->getPath().c_str(),
                                settingToString(*s).c_str());

                        root.remove(s->getIndex());
                        return;
                    }
                }
            } else {
                // List or array elements
                s = &root.add(source.getType());
            }
        } else {
            s = &root;
        }

        switch (source.getType())
        {
            case Setting::Type::TypeInt: {
                *s = (int)source;
                break;
            }

            case Setting::Type::TypeInt64: {
                *s = (long long)source;
                break;
            }

            case Setting::Type::TypeString: {
                *s = (const char*)source;
                break;
            }

            case Setting::Type::TypeBoolean: {
                *s = (bool)source;
                break;
            }

            case Setting::Type::TypeArray:
            case Setting::Type::TypeList:
            case Setting::Type::TypeGroup: {
                for (int i = 0; i < source.getLength(); ++i) {
                    _apply(*s, source[i], lockList);
                }
                break;
            }
        }
    }

    void apply(Setting& setting, LockList* lockList)
    {
        Config* config = Environment::getConfig();
        assert(config != nullptr);

        _apply(config->getRoot(), setting, lockList);
    }

}
}