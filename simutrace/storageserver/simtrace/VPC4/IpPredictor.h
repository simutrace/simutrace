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
#ifndef IP_PREDICTOR_H
#define IP_PREDICTOR_H

#include "Predictor.h"
#include "CompoundPredictor.h"

#include "FiniteContextMethodPredictor.h"

namespace SimuTrace
{

    template <typename T>
    class IpPredictor :
        public CompoundPredictor<T, 4>
    {
    private:
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

        inline void _getIp(PredictorId** codeBuffer, T** dataBuffer, T& out) 
        {
            PredictorId id = this->_readPredictorId(codeBuffer);

            if (id < 2)  {
                out = _firstOrderFcmPredictor.getValue(id);
            } else if (id < 4) {
                out = _thirdOrderFcmPredictor.getValue(id);
            } else {
                assert(id == IpPredictor::NotPredictedId);
                out = this->_readData(dataBuffer);
            }
        }

    public:
        IpPredictor() :
            _sharedFcmHistory(),
            _firstOrderFcmPredictor(0, _sharedFcmHistory),
            _thirdOrderFcmPredictor(2, _sharedFcmHistory) { }

        PredictorId encodeIp(PredictorId** codeBuffer, T** dataBuffer, 
                             const T ip)
        {
            PredictionContext<T> context;

            // Evaluate 1st and 3rd order predictors and update the shared
            // history afterwards.
            _firstOrderFcmPredictor.predictValue(context, ip);
            _thirdOrderFcmPredictor.predictValue(context, ip);

            _sharedFcmHistory.update(ip);

            return this->_evaluateContext(codeBuffer, dataBuffer, context, ip);
        }

        void decodeIp(PredictorId** codeBuffer, T** dataBuffer, T& out)
        {
            T result;

            // Get the value from the specified predictor
            _getIp(codeBuffer, dataBuffer, result);

            // Update the predictors and the history
            _firstOrderFcmPredictor.update(result);
            _thirdOrderFcmPredictor.update(result);

            _sharedFcmHistory.update(result);

            out = result;
        }

    };

}
#endif
