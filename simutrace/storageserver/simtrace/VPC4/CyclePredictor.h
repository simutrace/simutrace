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
#ifndef _CYCLE_PREDICTOR_H_
#define _CYCLE_PREDICTOR_H_

#include "Predictor.h"
#include "CompoundPredictor.h"

#include "FiniteContextMethodPredictor.h"

namespace SimuTrace
{

    template <typename T, typename K>
    class CyclePredictor :
        public CompoundPredictor<T, 4>
    {
    private:
        // We use a comparable predictor for cycles than for the ip.
        // According to [5.5 VPC3 and VPC4 Predictor Configurations], we use
        // a 1st and a 3rd order FCM for IP prediction with:
        //
        // a) a shared history for the 1st and 3rd order FCM predictors.
        //      -> we need a 3rd order history
        // b) 131.072 (2^17) lines in the 1st order FCM
        // c) 524.288 (2^19) lines in the 3rd order FCM
        // d) a history length of 2 values
        FcmHistory<T, 17, 3> _sharedFcmHistory;
        FiniteContextMethodPredictor<T, 17, 1, 2, T, 0, 17, 3> _firstOrderFcmPredictor;
        FiniteContextMethodPredictor<T, 19, 3, 2, T, 0, 17, 3> _thirdOrderFcmPredictor;

        T _referenceCycleCount;

        inline bool _getCycleCount(PredictorId** codeBuffer, T** dataBuffer, 
                                   T& out)
        {
            PredictorId id = this->_readPredictorId(codeBuffer);

            if (id < 2) {
                out = _firstOrderFcmPredictor.getValue(id);

                return true;
            } else if (id < 4) {
                out = _thirdOrderFcmPredictor.getValue(id);

                return true;
            } else {
                assert(id == CyclePredictor::NotPredictedId);
                out = this->_readData(dataBuffer);

                return false;
            }
        }
    public:
        CyclePredictor(T referenceCycleCount) :
            _sharedFcmHistory(),
            _firstOrderFcmPredictor(0, _sharedFcmHistory),
            _thirdOrderFcmPredictor(2, _sharedFcmHistory),
            _referenceCycleCount(referenceCycleCount) { }

        CyclePredictor() :
            CyclePredictor(0) { }

        PredictorId encodeCycle(PredictorId** codeBuffer, T** dataBuffer, 
                                const T cycle, const K ip)
        {
            PredictionContext<T> context;

            const T stride = cycle - _referenceCycleCount;
            const T value  = stride + ip;

            _referenceCycleCount = cycle;

            // Evaluate 1st and 3rd order predictors and update the shared
            // history afterwards.
            _firstOrderFcmPredictor.predictValue(context, value);
            _thirdOrderFcmPredictor.predictValue(context, value);

            _sharedFcmHistory.update(value);

            return this->_evaluateContext(codeBuffer, dataBuffer, context, stride);
        }

        void decodeCycle(PredictorId** codeBuffer, T** dataBuffer, const K ip, 
                         T& out)
        {
            T stride, update;

            if (_getCycleCount(codeBuffer, dataBuffer, stride)) {
                update = stride;
                stride -= ip;
            } else {
                update = stride + ip;
            }

            // Update the predictors and the history
            _firstOrderFcmPredictor.update(update);
            _thirdOrderFcmPredictor.update(update);

            _sharedFcmHistory.update(update);

            _referenceCycleCount += stride;

            out = _referenceCycleCount;
        }

        void setCycleCount(const T cycleCount) 
        {
            _referenceCycleCount = cycleCount;
        }
    };

}
#endif
