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
#ifndef KEYED_LAST_N_VALUE_PREDICTOR_H
#define KEYED_LAST_N_VALUE_PREDICTOR_H

#include "Predictor.h"

#include "FiniteContextMethodHistory.h"
#include "FiniteContextMethodPredictor.h"

namespace SimuTrace
{

    // Using an empty history for our finite context method predictor (FCM)
    // degrades the FCM to a keyed last n value predictor. The dummy history is
    // only used to transport the key to the underlying FCM.

    template<typename T, typename K, uint32_t tableSize, uint32_t lineLength>
    class KeyedLastNValuePredictor :
        public FiniteContextMethodPredictor<T, tableSize, 1, lineLength,
                                            K, 0, 0, 1>
    {
    private:
        KeyedFcmHistory<T, K, 0, 0, 1> _history;

    public:
        KeyedLastNValuePredictor(PredictorId idBase) :
            FiniteContextMethodPredictor<T, tableSize, 1, lineLength,
                                         K, 0, 0, 1>(idBase, _history),
            _history() { }

        void setKey(const K key)
        {
            // Instead of setting the key, we set the [1][1]-dimensional
            // history table to contain the key. The fcm predictor then uses
            // this value as index into its own table.
            _history.set(1, static_cast<HashType>(key));
        }

        const T getMostRecentValue() const
        {
            return this->getValue(this->_idBase);
        }

    };
}
#endif