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
#ifndef VALUE_PREDICTOR_H
#define VALUE_PREDICTOR_H

#include "Predictor.h"
#include "CompoundPredictor.h"

#include "FiniteContextMethodPredictor.h"
#include "KeyedLastNValuePredictor.h"

namespace SimuTrace
{

    template <typename T, typename K>
    class ValuePredictor :
        public CompoundPredictor<T, 10>
    {
    private:
        // According to [5.5 VPC3 and VPC4 Predictor Configurations], we
        // have the following setup of predictors:

        // Differential Finite Context Method Predictors (DFCM)
        // A 1st and a 3rd order DFCM with:
        // a) a shared history -> we need order 3 history
        // b) 65.536 (2^16) lines in the shared history
        // c) 131.072 (2^17) lines in the 1st order DFCM
        // d) 524.288 (2^19) lines in the 3rd order DFCM
        // e) a history of 2 values per DFCM
        KeyedFcmHistory<T, K, 16, 17, 3> _sharedDfcmHistory;
        FiniteContextMethodPredictor<T, 17, 1, 2, K, 16, 17, 3> _firstOrderDfcmPredictor;
        FiniteContextMethodPredictor<T, 19, 3, 2, K, 16, 17, 3> _thirdOrderDfcmPredictor;

        // Finite Context Method Predictor (FCM)
        // A single 1st order FCM with:
        // a) 65.536 (2^16) lines in the (private) history
        // b) 524.288 (2^19) lines in the FCM
        // c) a history of 2 values in the FCM
        KeyedFcmHistory<T, K, 16, 19, 1> _privateFcmHistory;
        FiniteContextMethodPredictor<T, 19, 1, 2, K, 16, 19, 1> _firstOrderFcmPredictor;

        // Last Value Predictor (LV)
        // A single last 4 value predictor with:
        // a) 65.536 (2^16) lines
        KeyedLastNValuePredictor<T, K, 16, 4> _last4ValuePredictor;

        inline void _setKey(K key)
        {
            _sharedDfcmHistory.setKey(key);
            _privateFcmHistory.setKey(key);
            _last4ValuePredictor.setKey(key);
        }

        inline void _getValue(PredictorId** codeBuffer, T** dataBuffer, T& out)
        {
            PredictorId id = this->_readPredictorId(codeBuffer);

            if (id < 2) {
                out = _firstOrderDfcmPredictor.getValue(id) +
                      _last4ValuePredictor.getMostRecentValue();
            } else if (id < 4) {
                out = _thirdOrderDfcmPredictor.getValue(id) +
                      _last4ValuePredictor.getMostRecentValue();
            } else if (id < 8) {
                out = _last4ValuePredictor.getValue(id);
            } else if (id < 10) {
                out = _firstOrderFcmPredictor.getValue(id);
            } else {
                assert(id == ValuePredictor::NotPredictedId);
                out = this->_readData(dataBuffer);
            }
        }

    public:
        ValuePredictor() :
            _sharedDfcmHistory(),
            _firstOrderDfcmPredictor(0, _sharedDfcmHistory),
            _thirdOrderDfcmPredictor(2, _sharedDfcmHistory),
            _privateFcmHistory(),
            _firstOrderFcmPredictor(8, _privateFcmHistory),
            _last4ValuePredictor(4) { }

        PredictorId encodeValue(PredictorId** codeBuffer, T** dataBuffer,
                                const T value, const K key)
        {
            PredictionContext<T> context;

            // Setup the predictors and histories to use the current key
            _setKey(key);

            // The last value needed to compute the stride is obtained from the
            // L4V predictor [5.5 VPC3 and VPC4 Predictor Configurations].
            T stride = value - _last4ValuePredictor.getMostRecentValue();

            // Evaluate the predictors
            _firstOrderDfcmPredictor.predictValue(context, stride);
            _thirdOrderDfcmPredictor.predictValue(context, stride);

            _last4ValuePredictor.predictValue(context, value);

            _firstOrderFcmPredictor.predictValue(context, value);

            // Now, that we gave all predictors a chance, we update the
            // history of the DFCMs and the FCM.
            _sharedDfcmHistory.update(stride);
            _privateFcmHistory.update(value);

            return this->_evaluateContext(codeBuffer, dataBuffer, context, value);
        }

        void decodeValue(PredictorId** codeBuffer, T** dataBuffer, const K key,
                         T& out)
        {
            T result;

            // Setup the predictors and histories to use the current key
            _setKey(key);

            // Get the value from the specified predictor
            _getValue(codeBuffer, dataBuffer, result);

            // Update the predictors and the histories
            T stride = result - _last4ValuePredictor.getMostRecentValue();
            _firstOrderDfcmPredictor.update(stride);
            _thirdOrderDfcmPredictor.update(stride);

            _last4ValuePredictor.update(result);

            _firstOrderFcmPredictor.update(result);

            // Now, that we updated all predictors, we can update the DFCM
            // and FCM history
            _sharedDfcmHistory.update(stride);
            _privateFcmHistory.update(result);

            out = result;
        }

    };
}
#endif
