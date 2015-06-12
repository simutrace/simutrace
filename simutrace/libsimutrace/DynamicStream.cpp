/*
 * Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus
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

#include "DynamicStream.h"

#include "ClientStream.h"

namespace SimuTrace
{

    DynamicStream::DynamicStream(StreamId id, const DynamicStreamDescriptor& desc,
                                 StreamBuffer& buffer,
                                 ClientSession& session) :
        ClientStream(id, desc.base, buffer, session),
        _desc(desc)
    {
        ThrowOn(isVariableEntrySize(desc.base.type.entrySize), Exception,
                "Variable entry size not supported for dynamic streams.");

        ThrowOnNull(_desc.operations.getNextEntry, Exception,
                    "A dynamic stream must own a getNextEntry handler.");

        // Give the caller a chance to initialize stream-local data.
        if (_desc.operations.initialize != nullptr) {
            int result = _desc.operations.initialize(&desc, id, &_desc.userData);
            ThrowOn(result != 0, UserCallbackException, result);
        }
    }

    DynamicStream::~DynamicStream()
    {
        if (_desc.operations.finalize != nullptr) {
            _desc.operations.finalize(getId(), _desc.userData);
        }
    }

    void DynamicStream::_initializeDynamicHandle(StreamHandle handle,
                                                 StreamStateFlags flags,
                                                 StreamAccessFlags aflags,
                                                 void* userData)
    {
        // ---------- This function must not throw (see _open()) --------------

        assert(handle != nullptr);

        handle->flags          = flags | StreamStateFlags::SsfDynamic;
        handle->accessFlags    = aflags;

        handle->stream         = this;
        handle->entrySize      = _desc.base.type.entrySize;

        // We set the values so that we will always fall in the segment load
        // path in StGetNextEntry() and StGetPreviousEntry()
        assert(getEntrySize(&_desc.base.type) >= 1);
        handle->entry          = nullptr;
        handle->segment        = INVALID_SEGMENT_ID;
        handle->segmentStart   = (byte*)1;
        handle->segmentEnd     = nullptr;

        handle->dyn.operations = &_desc.operations;
        handle->dyn.userData   = userData;
    }

    StreamHandle DynamicStream::_append(StreamHandle handle)
    {
        Throw(InvalidOperationException);
    }

    StreamHandle DynamicStream::_open(QueryIndexType type, uint64_t value,
                                      StreamAccessFlags flags, StreamHandle handle)
    {
    #ifdef _DEBUG
        if (handle != nullptr) {
            assert(handle->stream == this);
            assert(IsSet(handle->flags, StreamStateFlags::SsfRead));
            assert(IsSet(handle->flags, StreamStateFlags::SsfDynamic));
        }
    #endif

        // Specifying no flags and a handle will lead us to copy the
        // handle's flags. Otherwise, we use the specified flags.
        if ((handle != nullptr) && (flags == StreamAccessFlags::SafNone)) {
            flags = handle->accessFlags;
        } else {
            if (handle != nullptr) {
                handle->accessFlags = flags;
            }
        }

        void* userData = (handle != nullptr) ?
            handle->dyn.userData : nullptr;

        if (_desc.operations.open != nullptr) {
            // The dynamic stream can define a custom open handler. If this
            // handler fails, we will keep the handle intact and update the
            // user data. It may contain information that lets the user
            // retry the operation later.

            try {
                int result = _desc.operations.open(&_desc, getId(), type,
                                                   value, flags, &userData);
                ThrowOn(result != 0, UserCallbackException, result);
            } catch (...) {
                // We do not need to call the close operation of the handle
                // here, because:
                // We can get here only if the user callback directly throwed
                // an exception or if we ran into the ThrowOn. The first
                // should never happen legitimately, because the public API
                // does not throw. So the user throwed an exception and did
                // not catch it. This is a bug. In the case that we throwed
                // the UserCallbackException, the user reports us that the
                // open call failed. A close is not necessary. However, the
                // user might already updated (e.g., freed) the user data. We
                // therefore update it here.
                if (handle != nullptr) {
                    handle->dyn.userData = userData;
                }

                throw;
            }
        }

        if (handle == nullptr) {
            try {
                std::unique_ptr<StreamStateDescriptor> desc(
                    new StreamStateDescriptor());

                _initializeDynamicHandle(desc.get(), SsfRead, flags, userData);

                handle = desc.get();

                _addHandle(desc);
            } catch (...) {
                // The user possible saw the open operation, so we need to
                // call the close operation here.
                if (_desc.operations.close != nullptr) {
                    _desc.operations.close(getId(), &userData);
                }

                throw;
            }
        } else {
            // Must not throw, or we would need to call close()!
            _initializeDynamicHandle(handle, SsfRead, flags, userData);
        }

        LogDebug("Created new dynamic read handle for stream %d.", getId());

        return handle;
    }

    void DynamicStream::_closeHandle(StreamHandle handle)
    {
        assert(handle != nullptr);
        assert(handle->stream == this);
        assert(IsSet(handle->flags, StreamStateFlags::SsfDynamic));

        if (_desc.operations.close != nullptr) {
            _desc.operations.close(getId(), &handle->dyn.userData);
        }
    }

    void DynamicStream::queryInformation(StreamQueryInformation& informationOut) const
    {
        informationOut.descriptor = getDescriptor();
        informationOut.stats = StreamStatistics();
    }

}