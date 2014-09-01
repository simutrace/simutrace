# libsimutrace makefile
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
# Simutrace Client Library (libsimutrace) is part of Simutrace.
#
# libsimutrace is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libsimutrace is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libsimutrace. If not, see <http://www.gnu.org/licenses/>.
#

SOURCEFILES := main.cpp

# Session -----------------------------------
SOURCEFILES += ClientSession.cpp ClientSessionManager.cpp

# Store -------------------------------------
SOURCEFILES += ClientStore.cpp 

# Stream ------------------------------------
SOURCEFILES += ClientStream.cpp

# API ---------------------------------------
SOURCEFILES += SimuTrace.cpp

TARGETNAME := libsimutrace.so

include flags.mak

CONFIG ?= RELEASE

ALL_MACROS := $(COMMON_MACROS)

ifeq ($(CONFIG),DEBUG)
	BINARYDIR = Debug
	CFLAGS += $(DEBUG_CFLAGS)
	CXXFLAGS += $(DEBUG_CXXFLAGS)
	LDFLAGS += $(DEBUG_LDFLAGS)
	ALL_MACROS += $(DEBUG_MACROS)
endif

ifeq ($(CONFIG),RELEASE)
	BINARYDIR = Release
	CFLAGS += $(RELEASE_CFLAGS)
	CXXFLAGS += $(RELEASE_CXXFLAGS)
	LDFLAGS += $(RELEASE_LDFLAGS)
	ALL_MACROS += $(RELEASE_MACROS)
endif

OUTPUT_DIR := ../../build/$(BINARYDIR)

EXTERNAL_LIBS := ../libsimubase/$(BINARYDIR)/libsimubase.a ../libsimustor/$(BINARYDIR)/libsimustor.a

CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
CXXFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

CFLAGS += $(addprefix -D,$(ALL_MACROS))
CXXFLAGS += $(addprefix -D,$(ALL_MACROS))

LDFLAGS += $(addprefix -L,$(LIBRARY_DIRS))

LIBRARY_LDFLAGS = $(addprefix -l,$(LIBRARY_NAMES))

RPATH_PREFIX := -Wl,--rpath='$$ORIGIN/../
LIBRARY_LDFLAGS += $(EXTERNAL_LIBS)
LIBRARY_LDFLAGS += -Wl,--rpath='$$ORIGIN'
LIBRARY_LDFLAGS += $(addsuffix ',$(addprefix $(RPATH_PREFIX),$(dir $(EXTERNAL_LIBS))))
LIBRARY_LDFLAGS += -Wl,-soname,$(TARGETNAME)

all_make_files := Makefile flags.mak

source_obj1 := $(SOURCEFILES:.cpp=.o)
source_obj2 := $(source_obj1:.c=.o)
source_objs := $(source_obj2:.S=.o)

all_objs := $(addprefix $(BINARYDIR)/, $(notdir $(source_objs)))

#
# Build rules -----------------------------------------------------------------
#

all: $(BINARYDIR)/$(TARGETNAME)
	$(CP) $(BINARYDIR)/$(TARGETNAME) $(OUTPUT_DIR)/$(TARGETNAME)

$(BINARYDIR)/$(TARGETNAME): $(all_objs) $(EXTERNAL_LIBS)
	$(LD) -shared -o $@ $(LDFLAGS) $(START_GROUP) $(all_objs) $(LIBRARY_LDFLAGS) $(END_GROUP) $(LDFLAGS_END)

-include $(all_objs:.o=.dep)

clean:
	rm -rf $(BINARYDIR)
	rm -f $(OUTPUT_DIR)/$(TARGETNAME)

$(BINARYDIR):
	mkdir $(BINARYDIR)
	mkdir -p $(OUTPUT_DIR)

$(BINARYDIR)/%.o : %.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.S $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/libsimubase.a : ../libsimubase/$(BINARYDIR)/libsimubase.a
	$(CP) ../libsimubase/$(BINARYDIR)/libsimubase.a $(BINARYDIR)/libsimubase.a

$(BINARYDIR)/libsimustor.a : ../libsimustor/$(BINARYDIR)/libsimustor.a
	$(CP) ../libsimustor/$(BINARYDIR)/libsimustor.a $(BINARYDIR)/libsimustor.a