/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Client Library (libsimutrace) is part of Simutrace.
 *
 * libsimutrace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimutrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimutrace. If not, see <http://www.gnu.org/licenses/>.
 */
#include "SimuStor.h"
#include "Version.h"

#include "SimuTrace.h"

#include "ClientSessionManager.h"
#include "ClientSession.h"
#include "ClientStream.h"

namespace SimuTrace
{

    ClientSessionManager* _sessionManager;

    __thread std::string* _lastErrorMessage = nullptr;
    __thread ExceptionSite _lastErrorSite   = EsUnknown;
    __thread ExceptionClass _lastErrorClass = EcUnknown;
    __thread int _lastError = 0;

    void _onProcessExit()
    {
        assert(_sessionManager != nullptr);

        // Close all sessions. If the client is well-behaving, no sessions are
        // left. However, not closing a session on exit is a common error that
        // we want to handle to prevent data loss.
        uint32_t count = _sessionManager->getOpenSessionsCount();
        if (count > 0) {
            LogWarn("%d pending session(s) detected.", count);
        }

        try {
            _sessionManager->close();

            delete _sessionManager;
            _sessionManager = nullptr;
        } catch (const std::exception& e) {
            LogError("Failed to clean up pending sessions. "
                     "Possible data loss. The exception is '%s'.", e.what());
        }
    }

    inline ClientSessionManager& _getSessionManager()
    {
        if (_sessionManager == nullptr) {
            _sessionManager = new ClientSessionManager();

            // On Windows the exit handler will always fail, because we are
            // called after all handles have been destroyed. We therefore
            // do not provide the cleanup feature on Windows.
            // An alternative would be to build a static library. The exit
            // handler of the process image is called before destroying
            // handles.
        #ifdef WIN32
        #else
            atexit(_onProcessExit);
        #endif
        }

        return *_sessionManager;
    }

    inline ClientSession& _getSession(SessionId id)
    {
        ClientSessionManager& manager = _getSessionManager();

        return static_cast<ClientSession&>(manager.getSession(id));
    }

    void _setLastError(ExceptionSite errorSite, ExceptionClass errorClass,
                       int errorCode)
    {
        _lastErrorSite  = errorSite;
        _lastErrorClass = errorClass;
        _lastError      = errorCode;
    }

    void _setLastErrorMessage(const std::string* message)
    {
        if (_lastErrorMessage != nullptr) {
            delete _lastErrorMessage;
            _lastErrorMessage = nullptr;
        }

        if (message == nullptr) {
            return;
        }

        _lastErrorMessage = new std::string(*message);
    }

    void _setLastErrorFromPeerException(const PeerException& e)
    {
        std::string msg = e.what();

        _setLastError(EsServer, e.getErrorClass(), e.getErrorCode());
        _setLastErrorMessage(&msg);
    }

    void _setLastErrorFromException(const Exception& e)
    {
        std::string msg = e.what();

        _setLastError(EsClient, e.getErrorClass(), e.getErrorCode());
        _setLastErrorMessage(&msg);
    }

    void _setLastErrorUnknown(const std::exception& e)
    {
        std::string msg = e.what();

        _setLastError(EsClient, EcUnknown, std::numeric_limits<int>::min());
        _setLastErrorMessage(&msg);
    }

    void _setLastErrorSuccess()
    {
        if ((_lastErrorSite != EsUnknown) ||
            (_lastErrorClass != EcUnknown) ||
            (_lastError != 0)) {

            _setLastError(EsUnknown, EcUnknown, 0);
            _setLastErrorMessage(nullptr);
        }
    }

#define API_TRY    try {
#define API_CATCH(resultVar, errorValue)                                      \
            _setLastErrorSuccess();                                           \
        } catch (const PeerException& e) {                                    \
            (resultVar) = (errorValue);                                       \
            _setLastErrorFromPeerException(e);                                \
        } catch (const Exception& e) {                                        \
            (resultVar) = (errorValue);                                       \
            _setLastErrorFromException(e);                                    \
        } catch (const std::exception& e) {                                   \
            (resultVar) = (errorValue);                                       \
            _setLastErrorUnknown(e);                                          \
        }


    /* Base API */

    const uint32_t StClientVersion = SIMUTRACE_VERSION;

    SIMUTRACE_API
    uint32_t StGetClientVersion()
    {
        return StClientVersion;
    }

    SIMUTRACE_API
    int StGetLastError(ExceptionInformation* informationOut)
    {
        if (informationOut != nullptr) {
            informationOut->code = _lastError;
            informationOut->site = _lastErrorSite;
            informationOut->type = _lastErrorClass;

            if (_lastErrorMessage != nullptr) {
                informationOut->message = _lastErrorMessage->c_str();
            } else {
                static char empty[1] = {0};
                informationOut->message = empty;
            }

        }

        return _lastError;
    }


    /* Session API */

    SIMUTRACE_API
    SessionId StSessionCreate(const char* server)
    {
        SessionId id = INVALID_SESSION_ID;

        API_TRY {
            ClientSessionManager& manager = _getSessionManager();

            id = manager.createSession(std::string(server));
        } API_CATCH(id, INVALID_SESSION_ID);

        return id;
    }

    SIMUTRACE_API
    _bool StSessionOpen(SessionId session)
    {
        _bool result = _true;

        API_TRY {
            ClientSessionManager& manager = _getSessionManager();

            manager.openLocalSession(session);
        } API_CATCH(result, _false);

        return result;
    }

    SIMUTRACE_API
    _bool StSessionClose(SessionId id)
    {
        _bool result = _true;

        API_TRY {
            ClientSessionManager& manager = _getSessionManager();
            Session& session = manager.getSession(id);

            // This function should only be called from a session participating
            // thread. We therefore call detach, instead of close. If the
            // current thread is not attached to the session, this will fail
            // and we might end up with a stale session that we have to clean
            // up on exit.
            session.detach();
        } API_CATCH(result, _false);

        return result;
    }

    SIMUTRACE_API
    _bool StSessionCreateStore(SessionId session, const char* specifier,
                               _bool alwaysCreate)
    {
        _bool result = _true;

        API_TRY {
            ClientSession& cs = _getSession(session);

            cs.createStore(std::string(specifier), (alwaysCreate == _true));
        } API_CATCH(result, _false);

        return result;
    }

    SIMUTRACE_API
    _bool StSessionOpenStore(SessionId session, const char* specifier)
    {
        _bool result = _true;

        API_TRY {
            ClientSession& cs = _getSession(session);

            cs.openStore(std::string(specifier));
        } API_CATCH(result, _false);

        return result;
    }

    SIMUTRACE_API
    _bool StSessionCloseStore(SessionId session)
    {
        _bool result = _true;

        API_TRY {
            ClientSession& cs = _getSession(session);

            cs.closeStore();
        } API_CATCH(result, _false);

        return result;
    }

    SIMUTRACE_API
    _bool StSessionSetConfiguration(SessionId session, const char* configuration)
    {
        _bool result = _true;

        API_TRY {
            ClientSession& cs = _getSession(session);

            cs.applySetting(std::string(configuration));
        } API_CATCH(result, _false);

        return result;
    }


    /* Stream API */

    void _makeStreamDescriptorFromType(const char* name,
                                       const StreamTypeDescriptor* type,
                                       StreamDescriptor* descOut)
    {
        assert(name != nullptr);
        assert(type != nullptr);
        assert(descOut != nullptr);

        // We only check here if we can set the given values in the
        // stream descriptor. All other validation is up to the stream
        // registration.
        size_t len = strlen(name);
        ThrowOn(len > MAX_STREAM_NAME_LENGTH, Exception,
                "The stream name exceeds the name length limit.");

        //Prior to setting any field, we completely reset the descriptor.
        memset(descOut, 0, sizeof(StreamDescriptor));

        descOut->hidden = _false;
        memcpy(descOut->name, name, len);

        descOut->type = *type;
    }

    SIMUTRACE_API
    _bool StMakeStreamDescriptor(const char* name, uint32_t entrySize,
                                 _bool temporalOrder, StreamDescriptor* descOut)
    {
        _bool result = _true;

        API_TRY {
            ThrowOnNull(name, ArgumentNullException);
            ThrowOnNull(descOut, ArgumentNullException);

            StreamTypeDescriptor type = {0};
            StreamTypeId id;
            generateGuid(id);

            // Initialize type descriptor
            type.temporalOrder = temporalOrder;
            type.arch32Bit     = _false;
            type.bigEndian     = _false;

            type.entrySize     = entrySize;
            type.id            = id;

            std::string sid = guidToString(id);
            static_assert(GUID_STRING_LENGTH <= MAX_STREAM_TYPE_NAME_LENGTH,
                          "The stream type name length is too short.");
            assert(sid.length() == GUID_STRING_LENGTH);

            memcpy(type.name, sid.c_str(), sid.length());

            _makeStreamDescriptorFromType(name, &type, descOut);
        } API_CATCH(result, _false);

        return result;
    }

    SIMUTRACE_API
    _bool StMakeStreamDescriptorFromType(const char* name,
                                         const StreamTypeDescriptor* type,
                                         StreamDescriptor* descOut)
    {
        _bool result = _true;

        API_TRY {
            ThrowOnNull(name, ArgumentNullException);
            ThrowOnNull(type, ArgumentNullException);
            ThrowOnNull(descOut, ArgumentNullException);

            _makeStreamDescriptorFromType(name, type, descOut);
        } API_CATCH(result, _false);

        return result;
    }

    const StreamTypeDescriptor* StStreamFindMemoryType(ArchitectureSize size,
        MemoryAccessType accessType, MemoryAddressType addressType,
        _bool hasData)
    {
        return streamFindMemoryType(size, accessType, addressType, hasData);
    }

    SIMUTRACE_API
    StreamId StStreamRegister(SessionId session, StreamDescriptor* desc)
    {
        StreamId id = INVALID_STREAM_ID;

        API_TRY {
            ClientSession& cs = _getSession(session);

            id = cs.registerStream(*desc);
        } API_CATCH(id, INVALID_STREAM_ID);

        return id;
    }

    SIMUTRACE_API
    int StStreamEnumerate(SessionId session, size_t bufferSize,
                          StreamId* streamIdsOut)
    {
        int result = 0;

        API_TRY {
            ClientSession& cs = _getSession(session);

            std::vector<StreamId> streams;
            cs.enumerateStreams(streams, false);

            if (streamIdsOut != nullptr) {
                const size_t size = streams.size() * sizeof(StreamId);
                ThrowOn(bufferSize < size, ArgumentException);

                memcpy(streamIdsOut, streams.data(), size);
            }

            result = static_cast<int>(streams.size());
        } API_CATCH(result, -1);

        return result;
    }

    SIMUTRACE_API
    _bool StStreamQuery(SessionId session, StreamId stream,
                        StreamQueryInformation* informationOut)
    {
        _bool result = _true;

        API_TRY {
            ThrowOnNull(informationOut, ArgumentNullException);
            ClientSession& cs = _getSession(session);

            Stream& cstr = cs.getStream(stream);
            cstr.queryInformation(*informationOut);
        } API_CATCH(result, _false);

        return result;
    }

    SIMUTRACE_API
    StreamHandle StStreamAppend(SessionId session, StreamId stream,
                                StreamHandle handle)
    {
        StreamHandle result = nullptr;

        API_TRY {
            ClientStream* cstream;
            if (handle == nullptr) {
                ClientSession& cs = _getSession(session);

                cstream = static_cast<ClientStream*>(&cs.getStream(stream));
            } else {
                assert(handle->stream != nullptr);
                assert(session == INVALID_SESSION_ID);
                assert(stream == INVALID_STREAM_ID);

                cstream = reinterpret_cast<ClientStream*>(handle->stream);
            }

            result = cstream->append(handle);
        } API_CATCH(result, nullptr);

        return result;
    }

    SIMUTRACE_API
    StreamHandle StStreamOpen(SessionId session, StreamId stream,
                              QueryIndexType type, uint64_t value,
                              StreamAccessFlags flags, StreamHandle handle)
    {
        StreamHandle result = nullptr;

        API_TRY {
            ClientStream* cstream;
            if (handle == nullptr) {
                ClientSession& cs = _getSession(session);

                cstream = static_cast<ClientStream*>(&cs.getStream(stream));
            } else {
                assert(handle->stream != nullptr);

                cstream = reinterpret_cast<ClientStream*>(handle->stream);

                assert((session == INVALID_SESSION_ID) ||
                       (session == cstream->getSession().getLocalId()));
                assert((stream == INVALID_STREAM_ID) ||
                       (stream == cstream->getId()));
            }

            result = cstream->open(type, value, flags, handle);
        } API_CATCH(result, handle);

        return result;
    }

    SIMUTRACE_API
    _bool StStreamClose(StreamHandle handle)
    {
        _bool result = _false;

        API_TRY {
            ThrowOnNull(handle, ArgumentNullException);
            assert(handle->stream != nullptr);

            ClientStream* cstream;
            cstream = reinterpret_cast<ClientStream*>(handle->stream);
            cstream->close(handle);

            result = _true;
        } API_CATCH(result, _false);

        return result;
    }


    /* Tracing API */

    SIMUTRACE_API
    void* StGetNextEntry(StreamHandle* handlePtr)
    {
        return StGetNextEntryFast(handlePtr);
    }

    SIMUTRACE_API
    void* StGetPreviousEntry(StreamHandle* handlePtr)
    {
        return StGetPreviousEntryFast(handlePtr);
    }

    SIMUTRACE_API
    void StSubmitEntry(StreamHandle handle)
    {
        StSubmitEntryFast(handle);
    }

    SIMUTRACE_API
    size_t StWriteVariableData(StreamHandle* handlePtr, byte* sourceBuffer,
                               size_t sourceLength, uint64_t* referenceOut)
    {
        assert(handlePtr != nullptr);
        StreamHandle handle = *handlePtr;
        assert(handle != nullptr);
        assert(handle->control != nullptr);
        assert(!handle->isReadOnly);
        assert(isVariableEntrySize(handle->entrySize));

        // If the source buffer is empty, we do not need to store anything
        //   and indicate that through the special raw entry index
        if (sourceLength == 0) {
            if (referenceOut != nullptr) {
                *referenceOut = VARIABLE_ENTRY_EMPTY_INDEX;
            }

            return 0;
        }

        assert(handle->stream != nullptr);
        ClientStream* strm = reinterpret_cast<ClientStream*>(handle->stream);

        assert(sourceBuffer != nullptr);
        const uint32_t sizeHint = getSizeHint(handle->entrySize);
        const size_t savedSourceLength = sourceLength;

        assert(sizeHint > 0);

        const StreamSegmentId sqn = handle->control->link.sequenceNumber;
        assert(sqn != INVALID_STREAM_SEGMENT_ID);

        const uint64_t index = (sqn * strm->getStreamBuffer().getSegmentSize()) /
                               sizeHint + handle->control->rawEntryCount;

        do {
            uint32_t count = 0;
            size_t bytesWritten;

            size_t length = ((size_t)handle->segmentEnd - (size_t)handle->entry);

            bytesWritten = writeVariableData(handle->entry, length,
                                             sourceBuffer, sourceLength,
                                             sizeHint, &count);

            assert(bytesWritten <= sourceLength);

            // If the segment was already full, we did not write any data and
            // must not count the entry to this segment. In addition, we want
            // to account the entry only to the first segment we use.
            if (count > 0) {

                // Is this the first time, we have written data?
                if (sourceLength == savedSourceLength) {
                    handle->control->entryCount++;
                }

                sourceLength -= bytesWritten;
                handle->control->rawEntryCount += count;

                if (sourceLength == 0) {
                    handle->entry += count * sizeHint;
                    break;
                }

                sourceBuffer += bytesWritten;
            } else {
                assert(bytesWritten == 0);
            }

            // The stream segment is full. We need to get a new one
            assert(handle->segmentEnd -
                   (handle->entry + count * sizeHint) < sizeHint);

            handle = StStreamAppend(INVALID_SESSION_ID, INVALID_STREAM_ID,
                                    handle);

            *handlePtr = handle;
            if (handle == nullptr) {
                // This is a bad situation. We could only write the data
                // partially. As we do not have any means to revert that
                // operation, we return the reference. The caller can check if
                // the operation finished successfully by checking the returned
                // bytes written. It is ok to call this function with the
                // remaining buffer (and a new valid handle). However, this
                // will distort the entry count statistics.
                break;
            }

        } while (true);

        if (referenceOut != nullptr) {
            *referenceOut = index;
        }

        return savedSourceLength - sourceLength;
    }

    SIMUTRACE_API
    size_t StReadVariableData(StreamHandle* handlePtr, uint64_t reference,
                              byte* destinationBuffer)
    {
        assert(handlePtr != nullptr);
        StreamHandle handle = *handlePtr;
        assert(handle != nullptr);
        assert(isVariableEntrySize(handle->entrySize));

        // Handle empty data reference
        if (reference == VARIABLE_ENTRY_EMPTY_INDEX) {
            return 0;
        }

        // Ensure that the handle points to the segment that we need
        assert(handle->stream != nullptr);
        ClientStream* strm = reinterpret_cast<ClientStream*>(handle->stream);

        const uint32_t sizeHint = getSizeHint(handle->entrySize);
        assert(sizeHint > 0);

        const size_t segmentMaxSize = strm->getStreamBuffer().getSegmentSize();
        StreamSegmentId rsqn = static_cast<StreamSegmentId>(
            (reference * sizeHint) / segmentMaxSize);
        uint64_t localRef = reference - rsqn * (segmentMaxSize / sizeHint);
        size_t len = 0;

        do {

            if ((rsqn != handle->sequenceNumber) ||
                (handle->control == nullptr)) {

                // We are not in the required segment. Try to get it from the
                // server, if it exists
                handle = StStreamOpen(INVALID_SESSION_ID, INVALID_STREAM_ID,
                                      QueryIndexType::QSequenceNumber, rsqn,
                                      handle->accessFlags, handle);

                *handlePtr = handle;
                if ((handle == nullptr) || (handle->control == nullptr)) {
                    return -1;
                }
            }

            byte* src = handle->segmentStart + localRef * sizeHint;
            size_t srcSize = handle->segmentEnd - src;

            size_t localLen;
            _bool finished = readVariableData(src, srcSize, destinationBuffer,
                                              &localLen);

            len += localLen;

            if (finished) {
                break;
            }

            // The variable data reaches over into the next segment. We
            // will continue reading.
            localRef = 0;
            rsqn++;

            destinationBuffer += localLen;

        } while (true);

        return len;
    }

}