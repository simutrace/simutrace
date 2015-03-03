/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Store Library (libsimustor) is part of Simutrace.
 *
 * libsimustor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimustor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimustor. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef STREAM_H
#define STREAM_H

#include "SimuBase.h"
#include "SimuStorTypes.h"

namespace SimuTrace
{

    class StreamBuffer;

    class Stream
    {
    private:
        DISABLE_COPY(Stream);

        std::string _name;

        StreamId _id;

        const StreamDescriptor _desc;

        StreamBuffer& _buffer;
    protected:
        byte* getBufferSegmentEnd(SegmentId segment) const;

    public:
        Stream(StreamId id, const StreamDescriptor& desc, StreamBuffer& buffer);
        virtual ~Stream();

        virtual void queryInformation(StreamQueryInformation& informationOut) const = 0;

        const std::string& getName() const;
        StreamId getId() const;

        bool isHidden() const;

        StreamBuffer& getStreamBuffer() const;

        const StreamTypeDescriptor& getType() const;
        const StreamDescriptor& getDescriptor() const;
    };

}

#endif