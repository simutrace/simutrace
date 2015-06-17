/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
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
#include "SimuStor.h"

#include "Simtrace3Encoder.h"

#include "../ServerStore.h"
#include "../ServerStream.h"
#include "../ServerStreamBuffer.h"
#include "../ScratchSegment.h"

#include "../StorageServer.h"
#include "../WorkItem.h"
#include "../WorkerPool.h"

#include "Simtrace3Store.h"
#include "Simtrace3Frame.h"

namespace SimuTrace {
namespace Simtrace
{

    struct Simtrace3Encoder::WorkerContext
    {
        Simtrace3Encoder& encoder;
        ServerStreamBuffer& buffer;
        SegmentId segment;
        StreamAccessFlags flags;
        StorageLocation* location;

        WorkerContext(Simtrace3Encoder& encoder,
                        ServerStreamBuffer& buffer,
                        SegmentId segment,
                        StreamAccessFlags flags,
                        StorageLocation* location = nullptr) :
            encoder(encoder),
            buffer(buffer),
            segment(segment),
            flags(flags),
            location(location) { }
    };

    Simtrace3Encoder::Simtrace3Encoder(ServerStore& store,
                                       const std::string& friendlyName,
                                       ServerStream* stream,
                                       bool needScratch) :
        StreamEncoder(store, friendlyName),
        _stream(stream),
        _needScratch(needScratch)
    {

    }

    Simtrace3Encoder::~Simtrace3Encoder()
    {

    }

    void Simtrace3Encoder::_writerMain(WorkItem<WorkerContext>& workItem,
                                       WorkerContext& context)
    {
        // -- This method is called in the context of a worker thread --
        SwapEnvironment(&StorageServer::getInstance().getEnvironment());
        Simtrace3Store& store = static_cast<Simtrace3Store&>(
            context.encoder._getStore());

        assert(context.encoder._stream != nullptr);
        ServerStream* stream = context.encoder._stream;
        ServerStreamBuffer& buffer = context.buffer;

        SegmentControlElement* ctrl =
            buffer.getControlElement(context.segment);

        assert(ctrl->link.stream == stream->getId());

        assert(context.location == nullptr);

        try {
            std::unique_ptr<ScratchSegment> target;
            if (context.encoder._needScratch) {
                target = std::unique_ptr<ScratchSegment>(new ScratchSegment());
            }

            // Create the frame and add the data attribute
            Simtrace3Frame frame(stream, ctrl);

            context.encoder._encode(frame, context.segment,
                                    ctrl->link.sequenceNumber, target.get());

            // Build a storage location and write the frame into the store
            auto location = context.encoder.makeStorageLocation(frame);
            Simtrace3StorageLocation* sim3location =
                static_cast<Simtrace3StorageLocation*>(location.get());

            sim3location->offset = store.commitFrame(frame);
            sim3location->size   = frame.getHeader().totalSize;

            // Everything went fine. Complete the given segment. This will
            // make the segment available for read access.
            stream->completeSegment(ctrl->link.sequenceNumber, &location);

        } catch (const std::exception& e) {
            std::string bids =
                ServerStreamBuffer::bufferIdToString(context.buffer.getId());

            LogError("<encoder: '%s'> Encoding of segment %d in buffer %s "
                        "failed <stream: %d, sqn: %d>. Exception: '%s'. "
                        "The data will be discarded.",
                        context.encoder.getFriendlyName().c_str(),
                        context.segment, bids.c_str(), stream->getId(),
                        ctrl->link.sequenceNumber, e.what());

            stream->completeSegment(ctrl->link.sequenceNumber, nullptr);
        }
    }

    void Simtrace3Encoder::_readerMain(WorkItem<WorkerContext>& workItem,
                                       WorkerContext& context)
    {
        // -- This method is called in the context of a global worker --
        // -- pool thread or a session worker thread.                 --

        SwapEnvironment(&StorageServer::getInstance().getEnvironment());

        assert(context.encoder._stream != nullptr);
        ServerStream* stream = context.encoder._stream;

        SegmentControlElement* ctrl =
            context.buffer.getControlElement(context.segment);

        assert(ctrl->link.stream == stream->getId());

        assert(context.location != nullptr);
        StorageLocation& location = *context.location;
        Simtrace3StorageLocation& storageLocation =
            static_cast<Simtrace3StorageLocation&>(location);

        try {
            context.encoder._decode(storageLocation, context.segment,
                                    ctrl->link.sequenceNumber);

            if (!IsSet(context.flags, StreamAccessFlags::SafSynchronous)) {
                // Everything went fine. Report the completion. This will
                // wake up any waiting clients.
                stream->completeSegment(ctrl->link.sequenceNumber);
            }

        } catch (const std::exception& e) {
            std::string bids =
                ServerStreamBuffer::bufferIdToString(context.buffer.getId());

            LogError("<encoder: '%s'> Decoding of segment %d in buffer %s "
                        "failed <stream: %d, sqn: %d>. Exception: '%s'.",
                        context.encoder.getFriendlyName().c_str(),
                        context.segment, bids.c_str(), stream->getId(),
                        ctrl->link.sequenceNumber, e.what());

            if (!IsSet(context.flags, StreamAccessFlags::SafSynchronous)) {
                stream->completeSegment(ctrl->link.sequenceNumber, nullptr);
            } else {
                throw;
            }
        }
    }

    ServerStream* Simtrace3Encoder::_getStream() const
    {
        return _stream;
    }

    bool Simtrace3Encoder::write(ServerStreamBuffer& buffer,
        SegmentId segment, std::unique_ptr<StorageLocation>& locationOut)
    {
        WorkerContext ctx(*this, buffer, segment, StreamAccessFlags::SafNone);

        std::unique_ptr<WorkItemBase> workItem(
            new WorkItem<WorkerContext>(_writerMain, ctx));

        // Hidden streams are usually the backbone for other encoders. Encoding
        // these gets highest priority before jobs on hidden streams starve
        // because of too many new jobs from the client and eventually livelock
        // the server.
        WorkQueue::Priority prio =
            IsSet(_getStream()->getDescriptor().flags, StreamFlags::SfHidden) ?
            WorkQueue::Priority::High :
            WorkQueue::Priority::Normal;

        StorageServer::getInstance().getWorkerPool().submitWork(workItem, prio);

        return false;
    }

    bool Simtrace3Encoder::read(ServerStreamBuffer& buffer,
                                SegmentId segment,
                                StreamAccessFlags flags,
                                StorageLocation& location,
                                bool prefetch)
    {
        WorkerContext ctx(*this, buffer, segment, flags, &location);

        if (IsSet(flags, StreamAccessFlags::SafSynchronous)) {
            WorkItem<WorkerContext> workItem(_readerMain, ctx);

            _readerMain(workItem, workItem.getArgument());

            return true;
        } else {
            std::unique_ptr<WorkItemBase> workItem(
                new WorkItem<WorkerContext>(_readerMain, ctx));

            WorkQueue::Priority prio =
                IsSet(_getStream()->getDescriptor().flags, StreamFlags::SfHidden) ?
                WorkQueue::Priority::High : ((prefetch) ?
                    WorkQueue::Priority::Low :
                    WorkQueue::Priority::Normal);

            StorageServer::getInstance().getWorkerPool().submitWork(workItem,
                                                                    prio);

            return false;
        }
    }

    std::unique_ptr<StorageLocation> Simtrace3Encoder::makeStorageLocation(
            Simtrace3Frame& frame)
    {
        return std::unique_ptr<StorageLocation>(
            new Simtrace3StorageLocation(frame));
    };

}
}