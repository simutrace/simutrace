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
#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "SimuStor.h"

namespace SimuTrace
{

    typedef uint32_t HashType;

#define INVALID_PREDICTOR_INDEX std::numeric_limits<uint8_t>::max()
    typedef uint8_t PredictorId;

    template<typename T> class Predictor;

    // In VPC, multiple predictors are used to predict a certain value
    // (e.g., Ip). If more than one predictor is correct, VPC picks the
    // predictor with the highest usage count [3.3 The VPC3 Algorithm] to
    // generate an output stream with high homogeneity. We thus send a
    // prediction context through all predictors and each predictor may set
    // itself as best predictor, if its usage count exceeds the current one.
    template<typename T>
    struct PredictionContext {
        uint64_t usageCount;

        Predictor<T>* predictor;
        PredictorId predictorId;

        PredictionContext() :
            usageCount(0),
            predictor(nullptr),
            predictorId(INVALID_PREDICTOR_INDEX) {}

        inline bool isPredicted() const
        {
            assert((predictor == nullptr) ||
                   (predictorId != INVALID_PREDICTOR_INDEX));

            return (predictor != nullptr);
        }
    };

    template<typename T>
    class Predictor
    {
    protected:
        PredictorId _idBase;

        std::unique_ptr<uint64_t[]> _usageCount;
    public:
        Predictor(PredictorId idBase) :
            _idBase(idBase),
            _usageCount(nullptr) { }

        Predictor(PredictorId idBase, uint32_t maxPredictors) :
            _idBase(idBase),
            _usageCount(new uint64_t[maxPredictors])
        {
            memset(_usageCount.get(), 0, sizeof(_usageCount));
        }

        void incrementUsageCount(PredictorId id)
        {
            assert((_usageCount != nullptr) && (id >= _idBase));
            _usageCount[id - _idBase]++;
        }
    };

}
#endif