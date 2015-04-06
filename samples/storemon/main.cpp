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

void initializeOptionParser(ez::ezOptionParser& parser)
{
    parser.overview = "Simutrace Store Monitor";
    parser.syntax   = "storemon [OPTIONS] store";
    parser.example  = "storemon -f 1 -s socket:10.1.1.10:5431 "
                      "simtrace:mystore.sim\n\n";

    parser.add("",
               false,
               0,
               0,
               "Prints the help message.",
               OPT_SHORT_PREFIX "h",
               OPT_LONG_PREFIX "help");

    parser.add("0",
               false,
               1,
               0,
               "The frequency in which to repeat the query in seconds.",
               OPT_SHORT_PREFIX "f");

    parser.add("local:/tmp/.simutrace",
               false,
               1,
               0,
               "The server connection specifier.",
               OPT_SHORT_PREFIX "s",
               OPT_LONG_PREFIX "server");
}

void printUsage(ez::ezOptionParser& parser)
{
    std::string usage;
    parser.getUsage(usage);
    std::cout << usage << std::endl;
}

std::string outputStreamHeader(StreamId id, bool isCsv)
{
    std::ostringstream out;
    std::string delim((isCsv) ? stringFormat("[%d];", id) : "  ");

    if (!isCsv) {
        out << std::setw( 4) << "Id" << delim
            << std::setw(20) << "Name" << delim
            << std::setw(10) << "Entry Size" << delim;
    }

    out << std::setw(15) << "Entries" << delim
        << std::setw(15) << "Raw Entries" << delim
        << std::setw(10) << "Size" << delim
        << std::setw(10) << "Comp. Size" << delim
        << std::setw(23) << "Size/s" << delim
        << std::setw(23) << "Comp. Size/s" << delim;

    return out.str();
}

std::string outputStreamStats(StreamId id, const StreamQueryInformation& info,
                              const StreamStatistics& lastInfo, int freq,
                              int secs, bool isCsv)
{
    std::ostringstream out;
    std::string delim((isCsv) ? ";" : "  ");

    uint32_t esize = getEntrySize(&info.descriptor.type);

    if (!isCsv) {
        std::ostringstream esizeStr;
        esizeStr << esize << ((isVariableEntrySize(info.descriptor.type.entrySize)) ? " (V)" : "");

        out << std::setw( 4) << id << delim
            << std::setw(20) << std::string(info.descriptor.name).substr(0, 20) << delim
            << std::setw(10) << esizeStr.str() << delim;
    }

    out << std::setw(15) << info.stats.entryCount << delim
        << std::setw(15) << info.stats.rawEntryCount << delim;

    uint64_t size  = info.stats.entryCount * esize;
    uint64_t csize = info.stats.compressedSize;

    if (isCsv) {
        out << std::setw(10) << (size) << delim
            << std::setw(10) << info.stats.compressedSize << delim
            << std::setw(23) << ((info.stats.entryCount - lastInfo.entryCount) * esize) / freq << delim
            << std::setw(23) << (info.stats.compressedSize - lastInfo.compressedSize) / freq << delim;
    } else {
        std::ostringstream srateStr, csrateStr;
        srateStr << sizeToString((info.stats.entryCount - lastInfo.entryCount) * esize, SizeUnit::SuMiB)
                 << " (" << sizeToString(size / (secs + 1), SizeUnit::SuMiB) << ")";
        csrateStr << sizeToString(info.stats.compressedSize - lastInfo.compressedSize, SizeUnit::SuMiB)
                  << " (" << sizeToString(csize / (secs + 1), SizeUnit::SuMiB) << ")";

        out << std::setw(10) << sizeToString(info.stats.entryCount * esize) << delim
            << std::setw(10) << sizeToString(info.stats.compressedSize) << delim
            << std::setw(23) << srateStr.str() << delim
            << std::setw(23) << csrateStr.str() << delim;
    }

    return out.str();
}


int main(int argc, const char *argv[])
{
    // This sample connects to a storage server and outputs statistics about
    // the supplied store and its streams. It is an example on how to monitor
    // the progress of a tracing session.
    //
    // Note: Start the monitor AFTER the tracing session has been started.
    // Otherwise, the tool will not work as expected. If the store does not
    // exist, the tool will create it and not just fail. In that case, the
    // tracer may fail if it tries to overwrite the new store.
    // If it just opens the store (which is still in write mode, because the
    // tool keeps it open), the tracer can create new streams and write data
    // to the store, but the tool will not become aware of the new streams.
    // These problems are well known and might be fixed in a future version.

    // Initialize the option parser, take the arguments and parse them
    ez::ezOptionParser options;
    initializeOptionParser(options);

    options.parse(argc, argv);

    if (options.isSet(OPT_LONG_PREFIX "help")) {
        printUsage(options);
        return 0;
    }

    if (options.lastArgs.size() < 1) {
        std::cout << "Error: Missing store specifier.\n\n";
        printUsage(options);
        return -1;
    }

    int freq;
    std::string server;
    std::string store(*options.lastArgs[0]);

    options.get(OPT_SHORT_PREFIX "f")->getInt(freq);
    options.get(OPT_SHORT_PREFIX "s")->getString(server);

    SessionId session = INVALID_SESSION_ID;
    int result = 0;

    try {
        std::cout << "Connecting to server '" << server << "'..." << std::endl;

        // Setup new session/open store ---------------------------------
        // To query the statistics of a store, we create a new session and open
        // the specified store. The server shares an open store with all
        // sessions. Therefore, we are able to see the tracing progress of
        // other sessions, by reading the stream statistics from the shared
        // store.
        session = StSessionCreate(server.c_str());
        if (session == INVALID_SESSION_ID) {
            printLastError();
            return -1;
        }

        std::cout << "Opening store '" << store << "'..." << std::endl;

        // Open the specified store
        _bool success = StSessionOpenStore(session, store.c_str());
        ThrowOn(!success, Exception);

        StreamStatistics lastStreamStats[10];
        memset(lastStreamStats, 0, sizeof(lastStreamStats));

        int rounds = 0;
        do {

        #if defined(_WIN32)
            std::system("cls");
        #else
            std::system("clear");
        #endif

            std::cout << outputStreamHeader(0, false) << std::endl;

            // Enumerate streams and query stream stats ---------------------
            // With StStreamEnumerate we can get a list of all valid stream
            // ids. These ids can then be fed into the query function to get
            // detailed information on the streams contained in the store.
            StreamId ids[10];
            int count = StStreamEnumerate(session, sizeof(ids), ids);
            ThrowOn(count == -1, Exception);

            for (int i = 0; i < count; ++i) {
                StreamQueryInformation info;

                success = StStreamQuery(session, ids[i], &info);
                ThrowOn(!success, Exception);

                std::cout << outputStreamStats(ids[i], info, lastStreamStats[i],
                                               freq, rounds * freq, false) << std::endl;

                lastStreamStats[i] = info.stats;
            }

            rounds++;
            ThreadBase::sleep(freq * 1000);
        } while (freq > 0);

    } catch (const Exception&) {
        printLastError();

        result = -1;
    }

    // Run down ---------------------------------------------------------------
    // Close the session and then finally exit the sample app.
    StSessionClose(session);

    return result;
}