/*
 * Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus
 *
 * Simutrace Extensions Library (libsimutraceX) is part of Simutrace.
 *
 * libsimutraceX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimutraceX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimutraceX. If not, see <http://www.gnu.org/licenses/>.
 */
#include "SimuTraceInternal.h"

#include "SimuTraceX.h"

#include "StreamMultiplexer.h"

namespace SimuTrace
{

    inline void _setLastErrorFromException(const Exception& e)
    {
        StSetLastError(e.getErrorClass(), e.getErrorCode(), e.what());
    }

    inline void _setLastErrorUnknown(const std::exception& e)
    {
        StSetLastError(ExceptionClass::EcUnknown,
                       std::numeric_limits<int>::min(), e.what());
    }

    inline void _setLastErrorSuccess()
    {
        StSetLastError(ExceptionClass::EcUnknown, 0, nullptr);
    }

#define API_TRY                                                               \
        try {
#define API_CATCH(resultVar, errorValue)                                      \
            _setLastErrorSuccess();                                           \
        } catch (const Exception& e) {                                        \
            (resultVar) = (errorValue);                                       \
            _setLastErrorFromException(e);                                    \
        } catch (const std::exception& e) {                                   \
            (resultVar) = (errorValue);                                       \
            _setLastErrorUnknown(e);                                          \
        }

    /* Helpers */

    SIMUTRACEX_API
    StreamId StXStreamFindByName(SessionId session, const char* name,
                                 StreamQueryInformation* info)
    {
        StreamId result = INVALID_STREAM_ID;

        API_TRY {
            ThrowOnNull(name, ArgumentNullException, "name");
            std::string sn(name);

            // Find out how many streams are registered, so we can allocate
            // a buffer that is large enough.
            int count = StStreamEnumerate(session, 0, NULL);
            ThrowOn(count == -1, SimutraceException);

            std::vector<StreamId> ids;
            ids.resize(count); // Allocate space for ids

            count = StStreamEnumerate(session, count * sizeof(StreamId),
                                      ids.data());
            ThrowOn(count == -1, SimutraceException);

            // If streams have been removed (although not supported in 3.2),
            // we resize the array so we do not access invalid stream ids.
            // If new streams have been registered in the meantime, we ignore
            // them.
            if (count < ids.size()) {
                ids.resize(count);
            }

            for (int i = 0; i < ids.size(); ++i) {
                StreamQueryInformation query;
                _bool success = StStreamQuery(session, ids[i], &query);
                ThrowOn(!success, SimutraceException);

                if (sn.compare(query.descriptor.name) == 0) {
                    result = ids[i];

                    if (info != nullptr) {
                        *info = query;
                    }

                    break;
                }
            }

            ThrowOn(result == INVALID_STREAM_ID, NotFoundException);
        } API_CATCH(result, INVALID_STREAM_ID);

        return result;
    }


    /* Stream Multiplexer */

    SIMUTRACEX_API
    StreamId StXMultiplexerCreate(SessionId session, const char* name,
                                  MultiplexingRule rule, MultiplexerFlags flags,
                                  StreamId* inputStreams, uint32_t count)
    {
        StreamId result = INVALID_STREAM_ID;

        API_TRY {
            ThrowOnNull(name, ArgumentNullException, "name");
            ThrowOnNull(inputStreams, ArgumentNullException, "inputStreams");
            ThrowOn(count < 2, ArgumentException, "count");

            std::string n(name);
            std::vector<StreamId> input(inputStreams, inputStreams + count);

            std::unique_ptr<StreamMultiplexer> multiplexer(
                new StreamMultiplexer(session, n, rule, flags, input));

            result = multiplexer->getStreamId();

            // If everything went fine and we have the id of the dynamic stream
            // that will implement the multiplexer, we can throw away the
            // pointer. Simutrace will call a finalizer for the multiplexer
            // when the stream is destroyed (i.e., the hosting store is closed).
            multiplexer.release();
        } API_CATCH(result, INVALID_STREAM_ID);

        return result;
    }

}