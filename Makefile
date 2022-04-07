###############################################################################
#         Name: Automatic Makefile
#       Author: Paolo Fabio Zaino
#      License: Copyright by Paolo Fabio Zaino, all rights reserved.
#		Distributed under MIT license (read the MIT license for details)
#
#  Description: This Makefile is intended to be reusable to build many types of
#		C Libraries and it will also build and execute automatically all
#		the provided Unit and Integration tests at every build.
#		Your can reuse it very easly in your own libraries, just copy it
#		in you rlibrary structure and change the parameters in the next
#		section "Manualpart of the Makefile", and you're ready to build!
###############################################################################

###############################################################################
# Initialisation (please check next section for configurable options!)

# Make sure that OS macro is always set on every platform:
# (if you are porting ZVector to a platform that does not
# support uname, then simply set OS from the command line)
ifeq ($(OS),)
OS:=$(shell uname -s)
endif
$(info Building on: $(OS))

#
###############################################################################

###############################################################################
# Manual Configuration part of the Makefile

# Work directory, usually you do not
# need to change this one:


WDIR:=$(shell pwd)
DESTDIR?=/usr/local/lib

# Configure desired compiler:
CC:=gcc

# Configure additional compiler and linker flags:
CFLAGS+=
LDFLAGS+=
# Flags to be used only for release or production builds:
P_CFLAGS:=-O3 -flto

# If you want to pass some MACROS to your code you can use the following
# variable just add your -D<MY_MACRO>:
CODE_MACROS+=

# Configure Library name:
LIBNAME:=zvector
# Configure desired directory where to store the compiled library:
LIBDIR:=lib

# Configure Library source directory and temporary object directory:
SRC:=src
OBJ:=o

# Configure Library build scripts dir (scripts required to build the library)
SCRIPTSDIR:=scripts

# Configure directory containing source Unit Test Files and Integration Test
# files and configure desired directory where to store compiled tests ready
# for execution:
TESTDIR:=tests
TESTBIN:=$(TESTDIR)/bin
#
##############################################################################
##############################################################################
# ZVect Extensions

# In this section of the Makefile you can configure which ZVector Library
# extensions you want to be built-in when compilin gthe library.
# If you want an extension enabled the set the corresponded variable to 1
# otherwise set it to 0.

# Which type of memory management functions do you want to use?
# 0 for standard CLib memcpy and memmove
# 1 for optimised ZVector memcpy and memmove
# Recommendation: Try 1 ONLY when compiling for embedded systems
# 		  or IoT applications where you won't have Linux and the
#		  glibc available. Otherwise you should stick to 0.
MEMX_METHOD:=0

# Do you want the library to be built to be thread safe? (and so it uses mutex
# etc)? If so, set the following variable to 1 to enable thread safe code or
# set it to 0 to disable the thread safe code within the library:
# Recommendation: If you need to squeeze as much performance as possible out
# 		  of ZVector and thread safety is not a requirement for you
#		  then disable this option (set it to 0 zero). It will make
#		  ZVector gain some extra performance.
THREAD_SAFE_BUILD:=1

# Do you want ZVector code to be fully reentrant?
# 0 for no full reentrant code
# 1 for yes full reentrant code
FULL_REENTRANT:=0

# Do you want the DMF (Data Manipulation Functions) extensions enabled?
# This extension enables functions like vect_swap that allows you to swap
# two elements of the same vector.
DMF_EXTENSIONS:=1

# Do you want the SFMD (Single Function Multiple Data) extension enabled?
# This extension provides ZVect functions that you can call to modify entire
# vectors using a single function call.
# If you want the SFMD extensions enabled the set the following variable to 1
# otherwise set it to 0
SFMD_EXTENSIONS:=1

#
###############################################################################

###############################################################################
# Automated part of th Makefile:

RVAL0:=

# Add Default flags for GCC if the user has not passed any:
ifeq ($(CC),gcc)
	ifeq ($(strip $(CFLAGS)),)
		CFLAGS+=-std=c99 -Wall -Wextra -I./src -I./tests -fstack-protector-strong
		P_CFLAGS+=
	endif
	ifeq ($(strip $(LDFLAGS)),)
		LDFLAGS+=
	endif
endif

# Add Default flags for CLANG if the user has not passed any:
ifeq ($(CC),clang)
	ifeq ($(strip $(CFLAGS)),)
		CFLAGS+=-std=c99 -Wall -Wextra -I./src -I./tests -fstack-protector-strong
		P_CFLAGS+=
	endif
	ifeq ($(strip $(LDFLAGS)),)
		LDFLAGS+=
	endif
endif

ifeq ($(OS),Windows_NT)
    CCFLAGS += -D WIN32
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CCFLAGS += -D AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            CCFLAGS += -D AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            CCFLAGS += -D IA32
        endif
    endif
else
    ifeq ($(OS),Linux)
		SHELL := /bin/bash
        CCFLAGS += -D LINUX
		RVAL0:=/usr/bin/chmod -Rf +x $(WDIR)/scripts/ 2>&1
    endif
    ifeq ($(OS),Darwin)
		SHELL := /bin/bash
        CCFLAGS += -D OSX
		RVAL0:=/bin/chmod -Rf +x $(WDIR)/scripts/ 2>&1
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CCFLAGS += -D AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CCFLAGS += -D IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CCFLAGS += -D ARM
    endif
endif

ifeq ($(THREAD_SAFE_BUILD),1)
	LDFLAGS+= -lpthread
	#CODE_MACROS+= -DTHREAD_SAFE
	RVAL1 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_THREAD_SAFE 1
else
RVAL1 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_THREAD_SAFE 0
endif

ifeq ($(SFMD_EXTENSIONS),1)
	#CODE_MACROS+= -DZVECT_DMF_EXTENSIONS
	RVAL2 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_DMF_EXTENSIONS 1
else
	RVAL2 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_DMF_EXTENSIONS 0
endif

ifeq ($(SFMD_EXTENSIONS),1)
	#CODE_MACROS+= -DZVECT_SFMD_EXTENSIONS
	RVAL3 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_SFMD_EXTENSIONS 1
else
	RVAL3 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_SFMD_EXTENSIONS 0
endif

ifeq ($(FULL_REENTRANT),1)
	#CODE_MACROS+= -DZVECT_SFMD_EXTENSIONS
	RVAL4 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_FULL_REENTRANT 1
else
	RVAL4 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_FULL_REENTRANT 0
endif

ifeq ($(MEMX_METHOD),1)
	#CODE_MACROS+= -DZVECT_SFMD_EXTENSIONS
	RVAL5 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_MEMX_METHOD 1
else
	RVAL5 = $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_MEMX_METHOD 0
endif

SRCF:=$(wildcard $(SRC)/*.c)
OSRCF:=$(sort $(SRCF))
OBJF:=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(OSRCF))

TESTSLIST:=$(wildcard $(TESTDIR)/*.c)
OTESTSLIST:=$(sort $(TESTSLIST))
TESTSRCS:=$(patsubst $(TESTDIR)%.c, %, $(OTESTSLIST))
#$(info "$(TESTSRCS)")

TESTBINS:=$(patsubst %.c, %, $(TESTSRCS))
#$(info "$(TESTBINS)")

LIBST:=$(LIBDIR)/lib$(LIBNAME).a
#
###############################################################################

###############################################################################
# Targets:

# Use "make all" to build both core library and tests for production
.PHONY: all
all: CFLAGS += $(P_CFLAGS)
all: core test

# Use "make clean" to clean your previous builds
.PHONY: clean
clean:
	$(RM) -r $(LIBDIR) $(OBJ) $(TESTDIR)/bin ./*.o

# Use "make configure" to set your configuration (useful to enable code in your IDE)
.PHONY: configure
configure: $(SRC)/$(LIBNAME)_config.h $(SCRIPTSDIR)/ux_set_extension
	@echo ----------------------------------------------------------------
	$(RVAL0) || :
	$(RVAL1) || :
	$(RVAL2) || :
	$(RVAL3) || :
	$(RVAL4) || :
	$(RVAL5) || :
	@echo ----------------------------------------------------------------

# Use "make core" to just build the library FOR PRODUCTION
.PHONY: core
core: configure $(LIBDIR) $(LIBST)

.PHONY: test
test: $(TESTDIR)/bin $(TESTBINS)

# Use "make tests" to build tests and run them
.PHONY: tests
tests: test
	$(info   )
	$(info ===========================)
	$(info Running all found tests... )
	$(info ===========================)
	for test in $(TESTBINS) ; do time ./$(TESTBIN)$$test ; done

# Use "make debug" to build code for gdb debugger
.PHONY: debug
debug: CFLAGS+= -ggdb3
debug: CODE_MACROS+= -DDEBUG
debug: core tests

# Use "make testing" to build code with sanitizers for testing purposes:
.PHONY: testing
testing: CFLAGS+= -ggdb3 -fsanitize=address -fsanitize=leak -fsanitize=undefined
testing: CODE_MACROS+= -DDEBUG
testing: core tests

# Use "make install" to build and install the core library
.PHONY: install
install: core
	sudo cp -f $(LIBST) $(DESTDIR)

$(OBJF): $(OSRCF)
	$(info  )
	$(info ===========================)
	$(info Building $@                )
	$(info ===========================)
#	. /opt/rh/devtoolset-10/enable
	$(CC) -c -o $@ $< $(CFLAGS) $(CODE_MACROS)
	@echo ""
	@echo "Check if the objcode has been built:"
	@ls -alh ./o/*
	@echo "==========================="
	@echo ""


$(LIBST): $(OBJ) $(OBJF)
	$(info  )
	$(info ===========================)
	$(info Building $(LIBNAME) library)
	$(info ===========================)
#	. /opt/rh/devtoolset-10/enable
	ar rcs $@ $(OBJF)
	@echo ""
	@echo "Check if the library has been built:"
	@ls -alh ./lib/*
	@echo "==========================="
	@echo ""

$(TESTBINS): $(TESTSRCS)
	$(info  )
	$(info ===========================)
	$(info Building test: $@          )
	$(info ===========================)
#	. /opt/rh/devtoolset-10/enable
	$(CC) $(CFLAGS) $(CODE_MACROS) $(TESTDIR)$@.c -I$(WDIR)/src -L$(WDIR)/$(LIBDIR) -l$(LIBNAME) $(LDFLAGS)  -o $(TESTBIN)$@

$(LIBDIR):
	[ ! -d $@ ] && mkdir -p $@

$(OBJ):
	[ ! -d $@ ] && mkdir -p $@

$(TESTDIR)/bin:
	$(info )
	$(info ===========================)
	$(info Building Test Automation dirs:)
	$(info ===========================)
	[ ! -d $(WDIR)/$@ ] && mkdir -p $(WDIR)/$@
	@echo "==========================="
	@echo ""

#
###############################################################################
