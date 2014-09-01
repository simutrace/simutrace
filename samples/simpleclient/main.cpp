/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
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
#include "SimuTrace.h"
#include "SimuBase.h"

using namespace SimuTrace;

static const int lengths[3] = { 155, 56, 82 };
static const char* strings[3] = {
    "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam "
    "nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam "
    "erat, sed diam voluptua.",

    "At vero eos et accusam et justo duo dolores et ea rebum.",

    "Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum "
    "dolor sit amet."
};

StreamId dataStream = INVALID_STREAM_ID;
StreamId stringStream = INVALID_STREAM_ID;

// This will be our custom trace entry with a data field and a 
// reference to variable data. We cannot store the (variable-sized)
// string in the entry itself, because Simutrace enforces fixed-sized
// entries. We thus have to make use of the variable-sized data support
// in Simutrace which uses references and dedicated streams.
struct MyEntry {
    uint64_t myData;
    uint64_t myStringReference;
};

static const uint64_t memoryEntriesCount = (64 MiB * 300) / sizeof(DataWrite64) + 100;
static const uint64_t customEntriesCount = (64 MiB) / sizeof(MyEntry);

std::string errorSiteToString(ExceptionSite site)
{
    switch (site) 
    {
        case EsServer: return "Server";
        case EsClient: return "Client";
        default:
            return "Unknown Source";
    }
}

std::string errorClassToString(ExceptionClass errorClass)
{
    switch (errorClass) 
    {
        case EcRuntime:         return "Runtime";
        case EcPlatform:        return "Platform";
        case EcPlatformNetwork: return "Platform Network";
        case EcNetwork:         return "Network";
        default:
            return "Unknown Class";
    }
}

void printLastError()
{
    ExceptionInformation info;
    StGetLastError(&info);

    std::cout << std::endl;
    std::cout << "<< Exception >>" << std::endl;
    std::cout << "[" << errorSiteToString(info.site) << ","
              << errorClassToString(info.type) << ","
              << info.code << "]: "
              << info.message << std::endl;
}

int memoryWriterThreadMain(Thread<SessionId>& thread) 
{
    std::cout << "[Sample] Writing pseudo memory reference log..." << std::endl;

    SessionId session = thread.getArgument();
    StreamHandle handle = nullptr;

    // We are running in a new thread and must first connect the current thread 
    // to our tracing session.
    BOOL success = StSessionOpen(session);
    if (!success) {
        printLastError();
        return -1;
    }

    do {
        // We are now connected to the session. The main thread already opened 
        // a store, so we can register streams. We want to use the built-in 
        // memory encoder. To get a compatible type we use 
        // StStreamFindMemoryType(). We choose a type that will use the 
        // DataWrite64 structure. Afterwards we can use the type to form the 
        // stream descriptor and register our memory stream.
        StreamDescriptor desc;
        const StreamTypeDescriptor* type = StStreamFindMemoryType(
            ArchitectureSize::As64Bit,
            MemoryAccessType::MatWrite,
            MemoryAddressType::AtPhysical, 
            TRUE);

        success = StMakeStreamDescriptorFromType("Memory Stream", type, &desc);
        if (!success) {
            printLastError();
            break;
        }

        StreamId stream = StStreamRegister(session, &desc);
        if (stream == INVALID_STREAM_ID) {
            printLastError();
            break;
        }

        // We now have a new stream. To write our test entries, we need to
        // create a write handle first.
        handle = StStreamAppend(session, stream, nullptr);
        if (handle == nullptr) {
            printLastError();
            break;
        }

        // Simutrace uses 64MiB stream segments. That means after 64MiB of 
        // trace data, the server exchanges our buffer and starts to process 
        // the written entries. We fill a couple of segments with test entries.
        for (uint64_t i = 0; i < memoryEntriesCount; ++i) {

            // (1) Get a pointer to the next entry
            DataWrite64* entry = reinterpret_cast<DataWrite64*>(
                StGetNextEntryFast(&handle));
            if (entry == nullptr) {
                printLastError();
                break;
            }

            // (2) Fill the entry with information on our fake memory access
            entry->metadata.cycleCount = i;
            entry->metadata.fullSize = 1;

            entry->ip = 0xFFFFF78000000000 + (i % 0x10);
            entry->address = i;
            entry->data.data64 = 0x0123456789ABCDEF * i;

            // (3) Mark the entry as completely written
            StSubmitEntryFast(handle);
        }

    } while (false);

    int result = (StGetLastError(nullptr) == 0) ? 0 : -1;

    // We have written our full trace to the memory stream. We now run down
    // this thread. We have to close the stream handle and our reference to
    // the tracing session.
    StStreamClose(handle);
    StSessionClose(session);

    std::cout << "[Sample] Finished writing pseudo memory reference log." 
              << std::endl;

    return result;
}

int customWriterThreadMain(SessionId session)
{
    std::cout << "[Sample] Writing custom data..." << std::endl;

    StreamHandle dataHandle = nullptr;
    StreamHandle stringHandle = nullptr;

    // We are running in the same thread that originally created the session. 
    // We therefore do not need to open the session.

    do {
        // We are now connected to the session. The main thread already opened 
        // a store, so we can register streams. We will create a stream for
        // our actual data entries and another one for variable-sized data to
        // hold the strings.
        StreamDescriptor desc;

        BOOL success = StMakeStreamDescriptor("My Data Stream", sizeof(MyEntry), 
                                              FALSE, &desc);
        if (!success) {
            printLastError();
            break;
        }

        dataStream = StStreamRegister(session, &desc);
        if (dataStream == INVALID_STREAM_ID) {
            printLastError();
            break;
        }

        success = StMakeStreamDescriptor("String Stream", 
                                         makeVariableEntrySize(16),
                                         FALSE, &desc);
        if (!success) {
            printLastError();
            break;
        }

        stringStream = StStreamRegister(session, &desc);
        if (stringStream == INVALID_STREAM_ID) {
            printLastError();
            break;
        }

        // We now have the new streams. To write our test entries, we need to
        // create the write handles first.
        dataHandle = StStreamAppend(session, dataStream, nullptr);
        if (dataHandle == nullptr) {
            printLastError();
            break;
        }

        stringHandle = StStreamAppend(session, stringStream, nullptr);
        if (stringHandle == nullptr) {
            printLastError();
            break;
        }

        // Simutrace uses 64MiB stream segments. That means after 64MiB of 
        // trace data, the server exchanges our buffer and starts to process 
        // the written entries. We fill a couple of segments with test entries.
        for (uint64_t i = 0; i < customEntriesCount; ++i) {

            // (1) Get a pointer to the next entry
            MyEntry* entry = reinterpret_cast<MyEntry*>(
                StGetNextEntryFast(&dataHandle));
            if (entry == nullptr) {
                printLastError();
                break;
            }

            // (2) Fill the entry with information on our fake event
            entry->myData = i;

            size_t len = StWriteVariableData(&stringHandle, 
                                             (byte*)strings[i % 3], 
                                             lengths[i % 3],
                                             &entry->myStringReference);
            if (len != lengths[i % 3]) {
                printLastError();
                break;
            }

            // (3) Mark the entry as completely written
            StSubmitEntryFast(dataHandle);
        }

    } while (false);

    int result = (StGetLastError(nullptr) == 0) ? 0 : -1;

    // We have written our full trace to the stream. Close the stream handles.
    StStreamClose(dataHandle);
    StStreamClose(stringHandle);

    std::cout << "[Sample] Finished writing custom data." << std::endl;

    return result;
}

bool shouldRetry()
{
    ExceptionInformation info;
    StGetLastError(&info);

    if (info.code == 0) {
        return false;
    }

    // We could not get a handle or the next entry. The server asynchronously 
    // processes the data which we wrote seconds ago. Each 64MiB segment of a 
    // stream can only be read again, when the encoder finished writing it to 
    // the store. We therefore check if there is still an operation pending on
    // the current segment and retry in that case. Otherwise, we might have 
    // reached the end of the stream and bail out.
    if ((info.type == ExceptionClass::EcRuntime) &&
        (info.code == OperationInProgressException::code)) {

        ThreadBase::sleep(3000);
        return true;
    } else if ((info.type == ExceptionClass::EcRuntime) &&
               (info.code == NotFoundException::code)) {
        return false;
    }

    return false;
}

int customReaderThreadMain(SessionId session)
{
    std::cout << "[Sample] Reading custom data..." << std::endl;

    StreamHandle dataHandle = nullptr;
    StreamHandle stringHandle = nullptr;

    // This reader is rather complex because we are trying to read the trace
    // while the server might still be processing our data. The complete retry
    // logic is not needed when we are opening an existing store.

    try {
        // We first need to create read handles for the custom data stream and
        // the string stream. We query for the first segment so that the
        // handles will point to the beginning of the streams. We use the
        // sequential scan flag to enable read-ahead and non-polluting caching.
        do {
            dataHandle = StStreamOpen(session, dataStream, 
                                        QueryIndexType::QSequenceNumber, 0, 
                                        StreamAccessFlags::SafSequentialScan,
                                        nullptr);
        } while (shouldRetry());
        ThrowOnNull(dataHandle, Exception);

        do {
            stringHandle = StStreamOpen(session, stringStream, 
                                        QueryIndexType::QSequenceNumber, 0, 
                                        StreamAccessFlags::SafSequentialScan,
                                        nullptr);
        } while (shouldRetry());
        ThrowOnNull(stringHandle, Exception);

        uint64_t i = 0;
        char buffer[256]; // We know that all strings will fit into this buffer.
                          // See the documentation of StReadVariableData on
                          // how to get the exact buffer size for an entry.

        while (true) {

            // (1) Get a pointer to the next entry
            MyEntry* entry = reinterpret_cast<MyEntry*>(
                StGetNextEntryFast(&dataHandle));
            if (entry == nullptr) {
                if (shouldRetry()) {
                    continue;
                }

                if (i < customEntriesCount) {
                    // This should never happen!
                    std::cout << "Custom data entries lost!!" << std::endl;
                    Throw(Exception);
                }

                ExceptionInformation info;
                StGetLastError(&info);

                // If we have reached the end of the stream (i.e., a not found
                // exception), we close the handles and return 0, otherwise -1.
                StStreamClose(dataHandle);
                StStreamClose(stringHandle);

                return ((info.type == ExceptionClass::EcRuntime) &&
                        (info.code == NotFoundException::code)) ? 0 : -1;
            }

            memset(buffer, 0, sizeof(buffer));

            size_t len;
            do {
                len = StReadVariableData(&stringHandle, 
                                         entry->myStringReference,
                                         buffer);

            } while ((len == -1) && (shouldRetry()));
            ThrowOn(len == -1, Exception);

            // (2) Check the information in our fake event
            if ((entry->myData != i) ||
                (len != lengths[i % 3]) ||
                (strcmp(buffer, strings[i % 3]) != 0)) {

                // This should never happen!
                std::cout << "Custom data mismatch!!" << std::endl;
                Throw(Exception);
            };

            // We do not need a submit, because we are only reading the data!
            i++;
        }

    } catch (const Exception&) {
        printLastError();
    }

    // We only come here if something went wrong.
    StStreamClose(dataHandle);
    StStreamClose(stringHandle);

    std::cout << "[Sample] Finished reading custom data." << std::endl;

    return -1;
}

int memoryReaderThreadMain(SessionId session, StreamId stream)
{
    std::cout << "[Sample] Reading memory reference log..." << std::endl;

    // In contrast to the customReaderThreadMain(), this routine is called when
    // the server is guaranteed to have finished all processing (due to the
    // reopen). We therefore do not need to integrate any retry mechanisms as
    // any failure will be of non-temporary nature.
    StreamHandle handle = nullptr;

    try {

        // First, open a handle to the memory stream, we start at the
        // beginning of the stream.
        handle = StStreamOpen(session, stream, QueryIndexType::QSequenceNumber,
                              0, StreamAccessFlags::SafSequentialScan, nullptr);
        ThrowOnNull(handle, Exception);

        uint64_t i = 0;
        while (true) {

            // (1) Get a pointer to the next entry
            DataWrite64* entry = reinterpret_cast<DataWrite64*>(
                StGetNextEntryFast(&handle));
            if (entry == nullptr) {
                ExceptionInformation info;
                StGetLastError(&info);

                if (i < memoryEntriesCount) {
                    // This should never happen!
                    std::cout << "Memory entries lost!!" << std::endl;
                    Throw(Exception);
                };

                // If we have reached the end of the stream (i.e., a not found
                // exception), we close the handle and return 0, otherwise -1.
                StStreamClose(handle);

                return ((info.type == ExceptionClass::EcRuntime) &&
                        (info.code == NotFoundException::code)) ? 0 : -1;
            }

            // (2) Check the information in our fake event
            if ((entry->metadata.cycleCount != i) ||
                (entry->metadata.fullSize != 1) ||
                (entry->ip != 0xFFFFF78000000000 + (i % 0x10)) ||
                (entry->address != i) ||
                (entry->data.data64 != 0x0123456789ABCDEF * i)) {

                // This should never happen!
                std::cout << "Memory data mismatch!!" << std::endl;
                Throw(Exception);
            };

            // We do not need a submit, because we are only reading the data!
            i++;
        }

    } catch (const Exception&) {
        printLastError();
    }

    // We only come here if something went wrong.
    StStreamClose(handle);

    std::cout << "[Sample] Finished reading memory reference log."
              << std::endl;

    return -1;
}

int main(int argc, char *argv[])
{
    Thread<SessionId> memoryWriter(memoryWriterThreadMain);
    SessionId session = INVALID_SESSION_ID;
    int result = 0;
    BOOL success;

    // This is a quick and dirty sample client for Simutrace that writes and
    // reads some data using various ways to demonstrate how to communicate
    // with the server and how to use the API. Have fun!

    try {
        std::cout << "[Sample] Connecting to server..." << std::endl;

        // Phase 1
        // (1.1) Open session and create store --------------------------------
        // Before we can perform any operations with Simutrace, we have to 
        // connect to the storage server. Before starting this application 
        // ensure that the server is running on the same host and is listening
        // on the default local binding (.simutrace). This binding works on 
        // Windows and Linux.
        session = StSessionCreate("local:/tmp/.simutrace");
        ThrowOn(session == INVALID_SESSION_ID, Exception);

        // We now have a connection to the server. The next step is to create a
        // new store that will receive the data we are about to write. We use
        // the native simtrace storage format for the store. For version 3.0, 
        // this is the only format supported.
        success = StSessionCreateStore(session, "simtrace:mystore.sim", TRUE);
        ThrowOn(!success, Exception);


        // (1.2) Write some data to the store ---------------------------------

        // We now want to simultaneously write data to a memory stream and to
        // two other streams. To do so, we create a new thread that will write
        // the memory entries. All other data will be written with the current
        // thread.
        memoryWriter.setArgument(session);
        memoryWriter.start();

        result = customWriterThreadMain(session);
        ThrowOn(result == -1, Exception);


        // (1.3) Read some data from the store --------------------------------
        // To verify that our data has been written to the store, we read it in
        // again and check all values.
        result = customReaderThreadMain(session);
        ThrowOn(result == -1, Exception);

        // (1.4) Run down -----------------------------------------------------
        std::cout << "[Sample] Closing connection to server..." << std::endl;

        // Wait for the memory writer thread to return. Afterwards we have 
        // finished our work with Simutrace. Time to close the main thread's 
        // reference to the session. This will completely close the session.
        memoryWriter.waitForThread();
        StSessionClose(session);


        // Phase 2
        std::cout << "[Sample] Connecting to server 2nd time..." << std::endl;

        // (2.1) Setup new session/open store ---------------------------------
        // We have not read the memory stream, yet. For the sake of it we now 
        // want to reopen the store and read the memory stream. We thus create
        // a new session with the server. We could have also kept the old 
        // session alive and only closed/reopened the store. 
        session = StSessionCreate("local:/tmp/.simutrace");
        if (session == INVALID_SESSION_ID) {
            printLastError();
            return -1;
        }

        do {
            // This time, we create the store with alwaysCreate = FALSE, which 
            // will open the existing store and not create a new one. Since the
            // server might still be busy running down the store due to our 
            // previous session close, we retry the operation if necessary.
            success = StSessionCreateStore(session, "simtrace:mystore.sim", 
                                           FALSE);
        } while (shouldRetry());
        ThrowOn(!success, Exception);


        // (2.2) Enumerate streams and find memory stream ---------------------
        // With StStreamEnumerate we can get a list of all valid stream ids. 
        // These ids can then be fed into the query function to get detailed 
        // information on the streams contained in the store.
        StreamId ids[10];
        int count = StStreamEnumerate(session, sizeof(ids), ids);
        ThrowOn(count == -1, Exception);

        StreamId memoryStream = INVALID_STREAM_ID;
        for (int i = 0; i < count; ++i) {
            StreamQueryInformation info;

            success = StStreamQuery(session, ids[i], &info);
            ThrowOn(!success, Exception);

            size_t esize = getEntrySize(&info.descriptor.type);

            std::cout << "--- Stream " << ids[i] << " ---" << std::endl
                << " Name: " << std::string(info.descriptor.name) << std::endl
                << " Type: " << std::string(info.descriptor.type.name) << std::endl
                << " Entries: " << info.stats.entryCount << std::endl
                << " Raw Entries: " << info.stats.rawEntryCount << std::endl
                << " Entry Size: " << sizeToString(esize) 
                    << ((isVariableEntrySize(info.descriptor.type.entrySize)) ?
                    " (Variable)" : "") << std::endl
                << " Size: " << sizeToString(info.stats.rawEntryCount * esize) << std::endl
                << " Size On Disk: " << 
                    sizeToString(info.stats.compressedSize) << std::endl;

            if (strcmp(info.descriptor.name, "Memory Stream") == 0) {
                memoryStream = ids[i];

                assert(info.stats.entryCount == memoryEntriesCount);
            }
        }

        if (memoryStream == INVALID_STREAM_ID) {
            // This should never happen!
            std::cout << "Memory stream not found!" << std::endl;
            Throw(Exception);
        }


        // (2.3) Verify the memory stream -------------------------------------
        // We now read the memory stream from disk. Due to the reopen, all
        // cached entries are guaranteed to be flushed out of memory.
        StopWatch watch;

        watch.start();

        result = memoryReaderThreadMain(session, memoryStream);
        ThrowOn(result == -1, Exception);

        watch.stop();
        const uint64_t size = memoryEntriesCount * sizeof(DataWrite64);
        const uint64_t speed = (uint64_t)((double)size / watch.getSeconds());

        std::cout << "Read " << sizeToString(size)
                  << " memory stream in " << watch.getSeconds() 
                  << " seconds: " << speed/(1 MiB) << " MiB/s" << std::endl; 

    } catch (const Exception&) {
        printLastError();

        result = -1;
    }

    // Run down ---------------------------------------------------------------
    // As previously, close the session and then finally exit the sample app.
    memoryWriter.waitForThread();
    StSessionClose(session);

    return result;
}