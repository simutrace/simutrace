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
#ifndef VPC_PREDICTOR_BASE_H
#define VPC_PREDICTOR_BASE_H

#include "SimuStor.h"

#include "Predictor.h"

// With half-byte encoding, each predictor id is encoded with 4 bits in the
// final output stream.
// Activate half-byte encoding to improve compression with fast methods such as
// lz4. Half-byte encoding only negligibly decreases the compression ratio for
// advanced compressors (e.g., lzma).
//#define VPC_HALF_BYTE_ENCODING

namespace SimuTrace
{

    template<typename T, PredictorId maxPredictor>
    class CompoundPredictor :
        Predictor<T>
    {
    private:
        bool _halfByte;

    protected:
        void _writePredictorId(PredictorId** codeBuffer, PredictorId id)
        {
        #ifdef VPC_HALF_BYTE_ENCODING
            if (_halfByte) {
                (**codeBuffer) = id;
            } else {
                (**codeBuffer) |= (id << 4);
                (*codeBuffer)++;
            }

            _halfByte = !_halfByte;
        #else
            assert(id <= NotPredictedId);

            (**codeBuffer) = id;
            (*codeBuffer)++;
        #endif
        }

        PredictorId _readPredictorId(PredictorId** codeBuffer)
        {
            assert((codeBuffer != nullptr) && (*codeBuffer != nullptr));

            PredictorId id;
        #ifdef VPC_HALF_BYTE_ENCODING
            if (_halfByte) {
                id = (**codeBuffer) & 0x0F;
            } else {
                id = (**codeBuffer) >> 4;
                (*codeBuffer)++;
            }

            _halfByte = !_halfByte;
        #else
            id = (**codeBuffer);
            (*codeBuffer)++;
        #endif

            assert(id <= NotPredictedId);
            return id;
        }

        void _writeData(T** dataBuffer, T data)
        {
            assert((dataBuffer != nullptr) && (*dataBuffer != nullptr));

            (**dataBuffer) = data;
            (*dataBuffer)++;
        }

        T _readData(T** dataBuffer)
        {
            assert((dataBuffer != nullptr) && (*dataBuffer != nullptr));

            T data = **dataBuffer;
            (*dataBuffer)++;

            return data;
        }

        PredictorId _evaluateContext(PredictorId** codeBuffer, T** dataBuffer,
                                     PredictionContext<T>& context, T data)
        {
            // If we could predict the value, we increment the usage count of
            // the actual used predictor and return its id.
            if (context.isPredicted()) {
                assert(context.predictor != nullptr);
                assert(context.predictorId < NotPredictedId);

            #ifdef SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE
                incrementUsageCount(context.predictorId);
            #endif

                context.predictor->incrementUsageCount(context.predictorId);
            } else {
            #ifdef SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE
                incrementUsageCount(NotPredictedId);
            #endif

                // We could not predict the value. Write the data to the data
                // buffer and move to the next buffer slot.
                _writeData(dataBuffer, data);

                context.predictorId = NotPredictedId;
            }

            // Encode the predictor id
            _writePredictorId(codeBuffer, context.predictorId);

            return context.predictorId;
        }

        void _reset()
        {
            _halfByte = true;
        }

    public:
    #ifdef SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE
        CompoundPredictor() :
            Predictor<T>(0, maxPredictor + 1),
            _halfByte(true) { }
    #else
        CompoundPredictor() :
            Predictor<T>(0),
            _halfByte(true) { }
    #endif

        static const PredictorId NotPredictedId = maxPredictor;

    #ifdef SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE
        void addToProfileContext(ProfileContext& context, const char* prefix)
        {
            for (int i = 0; i < maxPredictor + 1; ++i) {
                context.add(stringFormat("%s[%d]", prefix, i).c_str(),
                            _usageCount[i], false);
            }
        }
    #endif
    };

}
#endif
