/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Original VPC4 algorithm by Cornell Research Foundation, Inc
 * Prof. Martin Burtscher
 *
 * Simutrace Storage Server (storageserver) is part of Simutrace.
 *
 * storageserver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * storageserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with storageserver. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef FINITE_CONTEXT_METHOD_HISTORY_H
#define FINITE_CONTEXT_METHOD_HISTORY_H

#include "Predictor.h"

namespace SimuTrace
{

    template<typename T, typename K, uint32_t tableSize,
             uint32_t hashSize, uint32_t order>
    class KeyedFcmHistory
    {
    private:
        static const HashType _hashMask = (1 << hashSize) - 1;
        static const HashType _tableMask = (1 << tableSize) - 1;

        uint32_t _index;
        HashType _historyTable[1 << tableSize][order];

        inline HashType _hash(const T value) const
        {
            // A fold & mask hash using XOR
            HashType hash = 0;
            T tmp = value;

            while (tmp > 0) {
                hash ^= tmp;
                tmp = tmp >> hashSize;
            }

            return hash & _hashMask;
        }

    public:
        KeyedFcmHistory() :
            _index(0)
        {
            memset(_historyTable, 0, sizeof(_historyTable));
        }

        void setKey(const K key)
        {
            _index = static_cast<uint32_t>(key & _tableMask);
            assert(_index < (1 << tableSize));
        }

        HashType get(uint32_t predictorOrder) const
        {
            assert((predictorOrder >= 1) && (predictorOrder <= order));
            return _historyTable[this->_index][predictorOrder - 1];
        }

        void set(uint32_t predictorOrder, HashType value)
        {
            assert((predictorOrder >= 1) && (predictorOrder <= order));
            _historyTable[this->_index][predictorOrder - 1] = value;
        }

        void update(const T value)
        {
            HashType hash = _hash(value);

            for (int i = order - 1; i > 0; --i) {
                _historyTable[this->_index][i] =
                    (_historyTable[this->_index][i - 1] << 1) ^ hash;
            }

            _historyTable[this->_index][0] = hash;
        }

    };

    template<typename T, uint32_t hashSize, uint32_t order>
    using FcmHistory = KeyedFcmHistory<T, T, 0, hashSize, order>;

    template<typename T, typename hKey, uint32_t hTableSize,
             uint32_t hHashSize, uint32_t hOrder>
    using HistoryReference =
        ObjectReference<KeyedFcmHistory<T, hKey, hTableSize, hHashSize, hOrder>>;

}
#endif