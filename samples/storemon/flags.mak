# storemon makefile

CC := gcc
CXX := g++
LD := $(CXX)
AR := ar
OBJCOPY := objcopy
CP := cp

COMMON_MACROS := 
DEBUG_MACROS := DEBUG
RELEASE_MACROS := NDEBUG RELEASE

INCLUDE_DIRS := ../../simutrace/include/ ../../simutrace/include/simubase/ ../../simutrace/include/simutrace/
LIBRARY_DIRS := 
LIBRARY_NAMES := config++

CFLAGS := -ffunction-sections
DEBUG_CFLAGS := -O0 -ggdb
RELEASE_CFLAGS := -O3

CXXFLAGS := -std=c++11 $(CFLAGS)
DEBUG_CXXFLAGS := $(DEBUG_CFLAGS)
RELEASE_CXXFLAGS := $(RELEASE_CFLAGS)

LDFLAGS := -Wl,--gc-sections -pthread
LDFLAGS_END := -Wl,-lrt
DEBUG_LDFLAGS := 
RELEASE_LDGLAGS :=

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group