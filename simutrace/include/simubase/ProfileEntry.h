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
#ifndef PROFILE_ENTRY_H
#define PROFILE_ENTRY_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "FastDelegate.h"

namespace SimuTrace
{

    class ProfileEntry
    {
    private:
        std::string _name;

    public:
        ProfileEntry(const std::string& name) :
            _name(name) { }

        virtual ~ProfileEntry() { }

        virtual std::string collect() = 0;

        const std::string& getName() const
        {
            return _name;
        }
    };

    template<typename T>
    class ProfileVariableReference :
        public ProfileEntry
    {
    private:
        T _lastValue;
        T& _value;

        bool _delta;
    public:
        ProfileVariableReference(const std::string& name, T& variable,
                                 bool delta = false) :
            ProfileEntry(name),
            _lastValue(0),
            _value(variable),
            _delta(delta) { }

        T get() const
        {
            return _value;
        }

        virtual std::string collect() override
        {
            std::stringstream str;
            T val;

            if (_delta) {
                val = _lastValue - _value;
                _lastValue = _value;
            } else {
                val = _value;
            }

            str << val;

            return str.str();
        }
    };

    template<typename T>
    class ProfileVariable :
        public ProfileVariableReference<T>
    {
    private:
        T _variable;

    public:
        ProfileVariable(const std::string& name, bool delta = false) :
            ProfileVariableReference<T>(name, _variable, delta),
            _variable(0) { }

        void set(T value)
        {
            _variable = value;
        }

    };

    template<typename T, typename C>
    class ProfileMemberDelegate0 :
        public ProfileEntry
    {
    private:
        fastdelegate::FastDelegate0<T> _delegate;

    public:
        ProfileMemberDelegate0(const std::string& name, C& instance,
                               T (C::*func)()) :
            ProfileEntry(name),
            _delegate(&instance, func) {}

        ProfileMemberDelegate0(const std::string& name, C& instance,
                               T (C::*func)() const) :
            ProfileEntry(name),
            _delegate(&instance, func) {}

        T get() const
        {
            return _delegate();
        }

        virtual std::string collect() override
        {
            std::stringstream str;
            str << get();

            return str.str();
        }
    };

}

#endif