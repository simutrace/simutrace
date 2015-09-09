/*
 * Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus
 *
 * This test is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This test is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this test. If not, see <http://www.gnu.org/licenses/>.
 */
#include <string>
#include <iostream>
#include <assert.h>

#include "SimuTrace.h"

using namespace SimuTrace;

class SimutraceException {};
class TestFailException {};

#define ThrowOn(expr) \
    if (expr) { \
        throw SimutraceException(); \
    }

#define ThrowTestFailOn(expr) \
    if (expr) { \
        throw TestFailException(); \
    }

#define FailOn(expr, count) \
    if (expr) { \
        std::cout << "failed (count " << count << ")"; \
        assert(false); \
        return false; \
    }

struct TestCase {
    uint64_t d[2]; // Number of data entries to write
    uint64_t a[2]; // Number of "append, close" - cycles
};

#define one  0x01
#define much 0xe8ab9f
#define some 0x08
#define more 0xe8

struct TestConfiguration {
    TestCase cases[47];
    int num;
} conf = {
    { // No append close (ac)
      {{some, some},{0,0}},
      {{much, much},{0,0}},

      // Only with single append close
      {{   0,    0},{one ,0}},     // write nothing, ac, write nothing
      {{some,    0},{one ,0}},     // write some, ac, write nothing
      {{much,    0},{one ,0}},     // write much, ac, write nothing
      {{   0, some},{one ,0}},     // write nothing, ac, write some
      {{   0, much},{one ,0}},     // write nothing, ac, write much
      {{some, some},{one ,0}},     // write some, ac, write some
      {{much, much},{one ,0}},     // write much, ac, write much
      {{some, much},{one ,0}},     // write some, ac, write much
      {{much, some},{one ,0}},     // write much, ac, write some

      // With some append close
      {{   0,    0},{some,0}},     // write nothing, ac, write nothing
      {{some,    0},{some,0}},     // write some, ac, write nothing
      {{much,    0},{some,0}},     // write much, ac, write nothing
      {{   0, some},{some,0}},     // write nothing, ac, write some
      {{   0, much},{some,0}},     // write nothing, ac, write much
      {{some, some},{some,0}},     // write some, ac, write some
      {{much, much},{some,0}},     // write much, ac, write much
      {{some, much},{some,0}},     // write some, ac, write much
      {{much, some},{some,0}},     // write much, ac, write some

      // With more append close
      {{   0,    0},{more,0}},     // write nothing, ac, write nothing
      {{some,    0},{more,0}},     // write some, ac, write nothing
      {{much,    0},{more,0}},     // write much, ac, write nothing
      {{   0, some},{more,0}},     // write nothing, ac, write some
      {{   0, much},{more,0}},     // write nothing, ac, write much
      {{some, some},{more,0}},     // write some, ac, write some
      {{much, much},{more,0}},     // write much, ac, write much
      {{some, much},{more,0}},     // write some, ac, write much
      {{much, some},{more,0}},     // write much, ac, write some

      // With some append close at the beginning and one at the end
      {{   0,    0},{some,one}},   // write nothing, ac, write nothing, ac
      {{some,    0},{some,one}},   // write some, ac, write nothing, ac
      {{much,    0},{some,one}},   // write much, ac, write nothing, ac
      {{   0, some},{some,one}},   // write nothing, ac, write some, ac
      {{   0, much},{some,one}},   // write nothing, ac, write much, ac
      {{some, some},{some,one}},   // write some, ac, write some, ac
      {{much, much},{some,one}},   // write much, ac, write much, ac
      {{some, much},{some,one}},   // write some, ac, write much, ac
      {{much, some},{some,one}},   // write much, ac, write some, ac

      // With some append close at the beginning and some at the end
      {{   0,    0},{some,some}},  // write nothing, ac, write nothing, ac
      {{some,    0},{some,some}},  // write some, ac, write nothing, ac
      {{much,    0},{some,some}},  // write much, ac, write nothing, ac
      {{   0, some},{some,some}},  // write nothing, ac, write some, ac
      {{   0, much},{some,some}},  // write nothing, ac, write much, ac
      {{some, some},{some,some}},  // write some, ac, write some, ac
      {{much, much},{some,some}},  // write much, ac, write much, ac
      {{some, much},{some,some}},  // write some, ac, write much, ac
      {{much, some},{some,some}}   // write much, ac, write some, ac
    },
    47
};

template<bool hasData, typename T, typename T2>
class _Test {
private:
    static void _writeData(StreamHandle handle, uint64_t start, uint64_t end)
    {
        // Write some data
        for (uint64_t i = start; i < end; ++i) {

            // (1) Get a pointer to the next entry
            T* entry = reinterpret_cast<T*>(StGetNextEntryFast(&handle));
            ThrowOn(entry == nullptr);

            // (2) Fill the entry with information on our fake memory access
            entry->metadata.cycleCount = i;
            entry->metadata.fullSize = 1;

            entry->ip = (T2)(0xFFFFF78000000000 + (i % 0x10));
            entry->address = (T2)i;
            if (hasData) {
                entry->dataFields[1] = (T2)(0x0123456789ABCDEF * i);
            }

            // (3) Mark the entry as written
            StSubmitEntryFast(handle);
        }
    }

    static void _aca(SessionId session, StreamId stream, StreamHandle& handle,
                     uint64_t n)
    {
        if (handle != nullptr) {
            StStreamClose(handle);
            handle = nullptr;
        }

        // Perform some append/close operations
        for (int i = 0; i < n; ++i) {
            handle = StStreamAppend(session, stream, nullptr);
            ThrowOn(handle == nullptr);

            StStreamClose(handle);
            handle = nullptr;
        }
    }

    static bool _performTest(SessionId session, const TestCase& tcase)
    {
        _bool success;

        std::cout << "[Test] Configuring test for ACA pattern on "
                  << ((sizeof(T2) == 4) ? "32" : "64")
                  << " bit memory stream "
                  << ((hasData) ? "with data " : "") << "("
                  << tcase.d[0] << ";" << tcase.a[0] << ";"
                  << tcase.d[1] << ";" << tcase.a[1] << ")...";

        success = StSessionCreateStore(session, "simtrace:test.sim", _true);
        ThrowOn(!success);

        const StreamTypeDescriptor* typeDesc;
        typeDesc = StStreamFindMemoryType((sizeof(T2) == 4) ?
                                            ArchitectureSize::As32Bit :
                                            ArchitectureSize::As64Bit,
                                          MemoryAccessType::MatRead,
                                          MemoryAddressType::AtPhysical,
                                          (hasData) ? _true : _false);

        StreamDescriptor desc;
        success = StMakeStreamDescriptorFromType("memorystream", typeDesc, &desc);
        ThrowOn(!success);

        StreamId stream = StStreamRegister(session, &desc);
        ThrowOn(stream == INVALID_STREAM_ID);

        // Write some data with ACA pattern
        StreamHandle handle = nullptr;

        std::cout << "ok." << std::endl;
        std::cout << "[Test] Writing data with ACA pattern...";

        uint64_t count = 0;
        for (int i = 0; i < 2; ++i) {
            if (tcase.d[i] > 0) {
                handle = StStreamAppend(session, stream, handle);
                ThrowOn(handle == nullptr);

                _writeData(handle, count, count + tcase.d[i]);
                count += tcase.d[i];
            }

            _aca(session, stream, handle, tcase.a[i]);
        }

        // We have written the full trace. Perform a store close and reopen to
        // wait until all data has been written out.
        StSessionCloseStore(session);

        success = StSessionOpenStore(session, "simtrace:test.sim");
        ThrowOn(!success);

        handle = StStreamOpen(session, stream, QueryIndexType::QIndex, 0,
                              StreamAccessFlags::SafSequentialScan, nullptr);
        if (count == 0) {
            FailOn(handle != nullptr, -1);
        } else {
            ThrowOn(handle == nullptr);

            // Validate the data. All data should be visible and correct, holes
            // should be transparent for the reader.
            std::cout << "ok." << std::endl;
            std::cout << "[Test] Validating data...";

            for (uint64_t i = 0; i < count; ++i) {
                T* entry = reinterpret_cast<T*>(StGetNextEntryFast(&handle));
                ThrowOn(entry == nullptr);

                FailOn(entry->metadata.cycleCount != i, i);
                FailOn(entry->metadata.fullSize != 1, i);

                FailOn(entry->ip != (T2)(0xFFFFF78000000000 + (i % 0x10)), i);
                FailOn(entry->address != (T2)i, i);
                if (hasData) {
                    FailOn(entry->dataFields[1] != (T2)(0x0123456789ABCDEF * i), i);
                }
            }

            FailOn(StGetNextEntryFast(&handle) != nullptr, -1);
        }

        std::cout << "ok." << std::endl;

        StSessionCloseStore(session);

        return true;
    }

public:
    static bool run(SessionId session, const TestConfiguration& conf)
    {
        for (int i = 0; i < conf.num; ++i) {
            if (!_performTest(session, conf.cases[i])) {
                return false;
            }
        }

        return true;
    }
};


template<typename T> class Test {};
template<> class Test<Read32> : public _Test<false, Read32, uint32_t> {};
template<> class Test<Read64> : public _Test<false, Read64, uint64_t> {};
template<> class Test<DataRead32> : public _Test<true, DataRead32, uint32_t> {};
template<> class Test<DataRead64> : public _Test<true, DataRead64, uint64_t> {};

int main(int argc, char *argv[])
{
    int code = 0;
    SessionId session = INVALID_SESSION_ID;

    std::cout << "[Test] Connecting to server..." << std::endl;

    // The append close append (ACA) pattern tests if we can create holes in a
    // stream without breaking something. Every time we call append on a stream,
    // we are using a sequence number. If we close the stream without writing any
    // data to the sequence number, we should have a hole at that position.
    // When reading data back in later, the hole should not be visible.

    try {
        session = StSessionCreate("local:/tmp/.simutrace");
        ThrowOn(session == INVALID_SESSION_ID);

        ThrowTestFailOn(!Test<DataRead32>::run(session, conf));
        ThrowTestFailOn(!Test<DataRead64>::run(session, conf));
        ThrowTestFailOn(!Test<Read32>::run(session, conf));
        ThrowTestFailOn(!Test<Read64>::run(session, conf));

    } catch (SimutraceException) {
        ExceptionInformation info;
        StGetLastError(&info);

        std::cout << "Exception: '" << std::string(info.message)
                  << "', code " << info.code << std::endl;

        code = -1;
    } catch (TestFailException) {
        code = -1;
    }

    StSessionClose(session);

    return code;
}