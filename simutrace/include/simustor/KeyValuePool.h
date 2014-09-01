/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Store Library (libsimustor) is part of Simutrace.
 *
 * libsimustor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimustor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimustor. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef KEY_VALUE_POOL_H
#define KEY_VALUE_POOL_H

#include "SimuBase.h"

namespace SimuTrace
{

    class Stream;

    template<typename K>
    class KeyValuePool
    {
    private:   
        DISABLE_COPY(KeyValuePool<K>);

        Stream& _stream;

    public:
        KeyValuePool(Stream& stream) :
            _stream(stream) { }

        virtual ~KeyValuePool() { }

        Stream& getStream() const
        {
            return _stream;
        }
    };

}
#endif