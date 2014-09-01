# Simutrace makefile
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
#

.PHONY = configuration libsimubase libsimustor libsimutrace storageserver

MAJOR_VERSION := 3
MINOR_VERSION := 0
REVISION      := 0
VERSION := $(MAJOR_VERSION).$(MINOR_VERSION).$(REVISION)

LIBIF_MAJOR_VERSION := 3
LIBIF_MINOR_VERSION := 0
LIBIF_REVISION      := 0
LIBIF_VERSION := $(LIBIF_MAJOR_VERSION).$(LIBIF_MINOR_VERSION).$(LIBIF_REVISION)

CONFIG ?= RELEASE

ifeq ($(CONFIG),DEBUG)
	BINARYDIR = Debug
endif
ifeq ($(CONFIG),RELEASE)
	BINARYDIR = Release
endif

LIBDIR ?= /usr/local/lib/
BINDIR ?= /usr/local/bin/

DEPS	:= libsimubase libsimustor
TARGETS := libsimutrace storageserver
SAMPLES := simpleclient storemon

#
# Build rules -----------------------------------------------------------------
#

all: $(TARGETS) $(SAMPLES)

$(DEPS):
	$(MAKE) -C simutrace/$@

$(TARGETS): $(DEPS)
	$(MAKE) -C simutrace/$@

$(SAMPLES): $(TARGETS)
	$(MAKE) -C samples/$@

documentation:
	mkdir -p build/documentation
	doxygen simutrace/include/PublicApi.doxyfile
	cp -rv simutrace/documentation/theme/. build/documentation/html/ 


#
# make clean
#

clean: $(addsuffix _clean,$(DEPS)) $(addsuffix _clean,$(TARGETS)) $(addsuffix _sclean,$(SAMPLES)) documentation_clean

%_clean:
	$(MAKE) -C simutrace/$* clean

%_sclean:
	$(MAKE) -C samples/$* clean

documentation_clean:
	rm -rf build/documentation

#
# make install
#

install: libsimutrace_install storageserver_install

libsimutrace_install: build/$(BINARYDIR)/libsimutrace.so
	mkdir -p $(LIBDIR)
	install -m 644 build/$(BINARYDIR)/libsimutrace.so $(LIBDIR)/libsimutrace.so.$(LIBIF_VERSION)
	ln -f -s libsimutrace.so.$(LIBIF_VERSION) $(LIBDIR)/libsimutrace.so.$(LIBIF_MAJOR_VERSION)
	ln -f -s libsimutrace.so.$(LIBIF_VERSION) $(LIBDIR)/libsimutrace.so

storageserver_install: build/$(BINARYDIR)/simustore
	mkdir -p $(BINDIR)
	install -m 755 build/$(BINARYDIR)/simustore $(BINDIR)
	ln -f $(BINDIR)/simustore $(BINDIR)/simustore-$(VERSION)