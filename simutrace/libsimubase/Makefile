# libsimubase makefile
#
# Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
# Marc Rittinghaus, Thorsten Groeninger
#
#             _____ _                 __
#            / ___/(_)___ ___  __  __/ /__________ _________ 
#            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
#           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
#          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/ 
#                         http://simutrace.org
#
# Simutrace Base Library (libsimubase) is part of Simutrace.
#
# libsimubase is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libsimubase is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libsimubase. If not, see <http://www.gnu.org/licenses/>.
#

SOURCEFILES := main.cpp

# Network -----------------------------------
SOURCEFILES += Channel.cpp ChannelProvider.cpp LocalChannel.cpp SocketChannel.cpp
SOURCEFILES += Port.cpp ClientPort.cpp ServerPort.cpp
SOURCEFILES += RpcInterface.cpp

# Compression -------------------------------
SOURCEFILES += lzma/Alloc.c lzma/LzFind.c lzma/LzmaDec.c lzma/LzmaEnc.c lzma/LzmaLib.c
SOURCEFILES += Compression.cpp

# Hashing -----------------------------------
SOURCEFILES += murmurhash3/MurmurHash3.cpp
SOURCEFILES += Hash.cpp

# I/O ---------------------------------------
SOURCEFILES += Directory.cpp File.cpp

# Logging -----------------------------------
SOURCEFILES += LogCategory.cpp LogEvent.cpp LogPriority.cpp
SOURCEFILES += LogAppender.cpp OstreamLogAppender.cpp TerminalLogAppender.cpp
SOURCEFILES += SimpleLogLayout.cpp PatternLogLayout.cpp

# Memory ------------------------------------
SOURCEFILES += MemorySegment.cpp PrivateMemorySegment.cpp SharedMemorySegment.cpp FileBackedMemorySegment.cpp
SOURCEFILES += MemoryHelpers.cpp 

# Threading ---------------------------------
SOURCEFILES += ConditionVariable.cpp CriticalSection.cpp Event.cpp ReaderWriterLock.cpp
SOURCEFILES += Interlocked.cpp
SOURCEFILES += ThreadBase.cpp

# Time --------------------------------------
SOURCEFILES += Clock.cpp StopWatch.cpp

# Other -------------------------------------
SOURCEFILES += Configuration.cpp  Environment.cpp Exceptions.cpp Profiler.cpp SafeHandle.cpp Utils.cpp

TARGETNAME := libsimubase.a

include flags.mak

CONFIG ?= RELEASE

ALL_MACROS := $(COMMON_MACROS)

ifeq ($(CONFIG),DEBUG)
	BINARYDIR = Debug
	CFLAGS += $(DEBUG_CFLAGS)
	CXXFLAGS += $(DEBUG_CXXFLAGS)
	ALL_MACROS += $(DEBUG_MACROS)
endif

ifeq ($(CONFIG),RELEASE)
	BINARYDIR = Release
	CFLAGS += $(RELEASE_CFLAGS)
	CXXFLAGS += $(RELEASE_CXXFLAGS)
	ALL_MACROS += $(RELEASE_MACROS)
endif

CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
CXXFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

CFLAGS += $(addprefix -D,$(ALL_MACROS))
CXXFLAGS += $(addprefix -D,$(ALL_MACROS))

all_make_files := Makefile flags.mak

source_obj1 := $(SOURCEFILES:.cpp=.o)
source_obj2 := $(source_obj1:.c=.o)
source_objs := $(source_obj2:.S=.o)

all_objs := $(addprefix $(BINARYDIR)/, $(notdir $(source_objs)))

#
# Build rules -----------------------------------------------------------------
#

all: $(BINARYDIR)/$(TARGETNAME)

$(BINARYDIR)/$(TARGETNAME): $(all_objs)
	$(AR) -r $@ $^

-include $(all_objs:.o=.dep)

clean:
	rm -rf $(BINARYDIR)

$(BINARYDIR):
	mkdir $(BINARYDIR)

$(BINARYDIR)/%.o : %.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.S $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/blocksort.o : bzip2/blocksort.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/bzerror.o : bzip2/bzerror.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/bzlib.o : bzip2/bzlib.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/compress.o : bzip2/compress.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/crctable.o : bzip2/crctable.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/decompress.o : bzip2/decompress.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/huffman.o : bzip2/huffman.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/randtable.o : bzip2/randtable.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/lz4.o : lz4/lz4.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/Alloc.o : lzma/Alloc.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/LzFind.o : lzma/LzFind.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/LzmaDec.o : lzma/LzmaDec.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/LzmaEnc.o : lzma/LzmaEnc.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/LzmaLib.o : lzma/LzmaLib.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/MurmurHash3.o : murmurhash3/MurmurHash3.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)