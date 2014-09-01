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
#include "SimuBase.h"
#include "SimuStorTypes.h"

#include "Stream.h"

#include "StreamBuffer.h"

namespace SimuTrace
{

    Stream::Stream(StreamId id, const StreamDescriptor& desc, 
                   StreamBuffer& buffer) :
        _id(id),
        _buffer(buffer),
        _desc(desc)
    {
        ThrowOn(id == INVALID_STREAM_ID, ArgumentException);
        ThrowOn((getEntrySize(&_desc.type) == 0) ||
                (getEntrySize(&_desc.type) > buffer.getSegmentSize()), 
                Exception, "Invalid entry size specified.");

        ThrowOn(isVariableEntrySize(desc.type.entrySize) &&
                getEntrySize(&desc.type) > VARIABLE_ENTRY_MAX_SIZE, Exception,
                stringFormat("The specified variable entry size exceeds the "
                    "supported maximum of %d bytes.", VARIABLE_ENTRY_MAX_SIZE));

        ThrowOn(_desc.type.temporalOrder && 
                (getEntrySize(&_desc.type) < sizeof(CycleCount)), 
                Exception, "The specified type is marked as temporally "
                           "ordered, however, its too small to include "
                           "a time stamp.");

        // Automatically reading the cycle count from a segment is only
        // supported for fixed-size entries.
        ThrowOn(_desc.type.temporalOrder &&
                isVariableEntrySize(_desc.type.entrySize),
                Exception, "Variable-sized entries cannot be temporally "
                           "ordered.");

        ThrowOn((strlen(_desc.name) >= MAX_STREAM_NAME_LENGTH) ||
                (strlen(_desc.type.name) >= MAX_STREAM_TYPE_NAME_LENGTH),
                Exception, "The length of the stream or type name exceeds the "
                "defined maximum. Ensure that all strings are properly "
                "terminated.");

        _name = std::string(_desc.name);
    }

    Stream::~Stream()
    {

    }

    const std::string& Stream::getName() const
    {
        return _name;
    }

    StreamId Stream::getId() const
    {
        return _id;
    }

    bool Stream::isHidden() const
    {
        return (_desc.hidden == TRUE);
    }

    StreamBuffer& Stream::getStreamBuffer() const
    {
        return _buffer;
    }

    const StreamTypeDescriptor& Stream::getType() const
    {
        return _desc.type;
    }

    const StreamDescriptor& Stream::getDescriptor() const
    {
        return _desc;
    }

}