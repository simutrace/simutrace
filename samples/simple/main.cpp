/*
 * Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus
 *
 * This sample is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This sample is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this sample. If not, see <http://www.gnu.org/licenses/>.
 */
#include <string>
#include <iostream>

#include "SimuTrace.h"

using namespace SimuTrace;

// This will be our custom trace entry with two data fields
typedef struct _MyEntry {
    uint64_t myData;
    uint64_t myData2;
} MyEntry;

#define ThrowOn(expr) if (expr) { throw std::exception(); }

int main(int argc, char *argv[])
{
    SessionId session = INVALID_SESSION_ID;
    StreamHandle handle = nullptr;

    // This is a quick and dirty sample client for Simutrace that writes and
    // reads some data to demonstrate how to communicate with the server and
    // how to use the API. Have fun!

    std::cout << "[Sample] Connecting to server..." << std::endl;

    try {
        _bool success;

        // 1. Open session and create store -----------------------------------
        // Before we can perform any operations with Simutrace, we have to
        // connect to the storage server. Before starting this application
        // ensure that the server is running on the same host and is listening
        // on the default local binding (.simutrace). This binding works on
        // Windows and Linux.
        SessionId session = StSessionCreate("local:/tmp/.simutrace");
        ThrowOn(session == INVALID_SESSION_ID);

        // We now have a connection to the server. The next step is to create a
        // new store that will receive the data we are about to write. We use
        // the native simtrace storage format for the store.
        success = StSessionCreateStore(session, "simtrace:mystore.sim", _true);
        ThrowOn(!success);

        // 2. Write some data to the store ------------------------------------
        std::cout << "[Sample] Writing custom data..." << std::endl;

        // We are now connected to the session. We already opened a store, so
        // we can register streams. We will create a stream to hold our entries.
        // The first step here is to describe the stream we want to register.
        // This includes defining the type of the entries as well as providing
        // a name for the stream.
        StreamDescriptor desc;

        success = StMakeStreamDescriptor("My Data Stream", sizeof(MyEntry),
                                         _false, &desc);
        ThrowOn(!success);

        StreamId stream = StStreamRegister(session, &desc);
        ThrowOn(stream == INVALID_STREAM_ID);

        // We now have the new stream. To write our test entries, we need to
        // create a write handle first.
        handle = StStreamAppend(session, stream, nullptr);
        ThrowOn(handle == nullptr);

        // Simutrace uses 64MiB stream segments. That means after 64MiB of
        // trace data, the server exchanges our buffer and starts to process
        // the written entries. We fill a couple of segments with test entries.
        for (uint64_t i = 0; i < 0xffffff; ++i) {

            // (1) Get a pointer to the next entry
            MyEntry* entry = reinterpret_cast<MyEntry*>(
                StGetNextEntryFast(&handle));
            ThrowOn(entry == nullptr);

            // (2) Fill the entry with information on our fake event
            entry->myData  = i;
            entry->myData2 = i + 1;

            // (3) Mark the entry as completely written
            StSubmitEntryFast(handle);
        }

        // We have written our full trace to the stream. Close the stream handle.
        std::cout << "[Sample] Finished writing custom data." << std::endl;

    } catch (std::exception) {
        ExceptionInformation info;
        StGetLastError(&info);

        std::cout << "Exception: '" << std::string(info.message)
                  << "', code " << info.code << std::endl;
    }

    // 3. Run down ------------------------------------------------------------
    // Close open handles and the session and then exit the sample app.
    StStreamClose(handle);
    StSessionClose(session);

    return 0;
}