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
#ifndef FINITE_CONTEXT_METHOD_PREDICTOR_H
#define FINITE_CONTEXT_METHOD_PREDICTOR_H

#include "Predictor.h"

#include "FiniteContextMethodHistory.h"

namespace SimuTrace
{

    //
    // The finite context method predictor (FCM) takes a history of the last n
    // observed symbols (where n denotes the predictor's order) and predicts
    // the symbol which followed the observed sequence the last time. FCMs thus
    // are good at predicting previously observed sequences. To simplify
    // matching the history, the input symbols are hashed and used as an index
    // into a table containing the prediction (i.e., the value encountered the
    // last time for the given history). In this implementation, for each
    // prediction an additional history is kept to allow the predictor to
    // predict not only the least recent value which followed the input
    // sequence the last time, but also the second recent value and so on.
    // Thus, our FCM technically maintains a last value predictor for each
    // hashed input sequence.
    //
    // To allow multiple FCMs to share a common input sequence history, the
    // history implementation is encapsulated in the FcmHistory classes. If no
    // history is specified in the constructor, a simple history of the
    // specified order is created.
    //

    // T: symbol type
    // tableSize: size of the hash table
    // order: length of the input history to match against
    // lineLength: length of the prediction history for each input history.
    //             For each input, the predictor will provide lineLength
    //             predictions.
    template<typename T, uint32_t tableSize, uint32_t order, uint32_t lineLength,
             typename hKey, uint32_t hTableSize, uint32_t hHashSize, uint32_t hOrder>
    class FiniteContextMethodPredictor :
        public Predictor<T>
    {
    private:
        typedef KeyedFcmHistory<T, hKey, hTableSize, hHashSize, hOrder> History;
    private:
        static const uint32_t _tableMask = (1 << tableSize) - 1;

        T _valueTable[1 << tableSize][lineLength];
        HistoryReference<T, hKey, hTableSize, hHashSize, hOrder> _history;

        inline HashType _mask(HashType index) const
        {
            return index & _tableMask;
        }

        void _initialize()
        {
            // It does not make sense to initialize the prediction history to
            // all 0s, as we would then predict the same value multiple times.
            for (uint32_t index = 0; index < (1 << tableSize); ++index) {
                for (uint32_t i = 0; i < lineLength; ++i) {
                    _valueTable[index][i] = i;
                }
            }
        }

        inline void _update(HashType index, T newValue)
        {
            assert(index < (1 << tableSize));

            // VPC4 only updates the value history if the first value is
            // different from the input to avoid an unnecessary insertion of
            // the same value. A full compare is not done to prevent the
            // decoding step from having to check against all entries in all
            // predictors to determine if an update should be performed.
            if (_valueTable[index][0] != newValue) {
                // Shift the value history to make room for the new value.
                for (int i = lineLength - 1; i > 0; --i) {
                    _valueTable[index][i] = _valueTable[index][i - 1];
                }

                _valueTable[index][0] = newValue;
            }
        }

    public:
        FiniteContextMethodPredictor(PredictorId idBase) :
            Predictor<T>(idBase, lineLength),
            _history(new FcmHistory<T, tableSize, order>(),
                     [](FcmHistory<T, tableSize, order>* instance)
                     { delete instance; })
        {
            _initialize();
        }

        FiniteContextMethodPredictor(PredictorId idBase, History& history) :
            Predictor<T>(idBase, lineLength),
            _history(&history, NullDeleter<History>::deleter)
        {
            _initialize();
        }

        void predictValue(PredictionContext<T>& context, const T value)
        {
            assert(_history != nullptr);
            HashType index = _mask(_history->get(order));

            // We iterate over the value history and perform a prediction check
            // for each element in the value history. This way, we predict the
            // most recent value, the second recent value, etc.
            assert(index < (1 << tableSize));
            for (int i = 0; i < lineLength; ++i) {
                if ((this->_usageCount[i] >= context.usageCount) &&
                    (_valueTable[index][i] == value)) {

                    context.predictor   = this;
                    context.predictorId = i + this->_idBase;

                    // cache the usage count
                    context.usageCount  = this->_usageCount[i];
                }
            }

            _update(index, value);
        }

        const T getValue(PredictorId id) const
        {
            assert(_history != nullptr);
            HashType index = _mask(_history->get(order));

            assert((id >= this->_idBase) && ((id - this->_idBase) < lineLength));
            return _valueTable[index][id - this->_idBase];
        }

        void update(const T value)
        {
            assert(_history != nullptr);
            HashType index = _mask(_history->get(order));

            _update(index, value);
        }

    };

}
#endif