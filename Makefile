###############################################################################
#        Name: Automatic Makefile
#      Author: Paolo Fabio Zaino
#     License: Copyright by Paolo Fabio Ziano, all rights reserved.
# 			   Distributed under MIT license (read the MIT license for details)
#
# Description: This Makefile is intended to be reusable to build many types of
#			   C Libraries and it will also build and execute automatically all
#			   the provided Unit and Integration tests at every build.
#			   Your can reuse it very easly in your own libraries, just copy it
#              in you rlibrary structure and change the parameters in the next
#			   section "Manualpart of the Makefile", and you're ready to build!
###############################################################################

###############################################################################
# Manual part of the Makefile

WDIR=$(shell pwd)

# Configure desired compiler:
CC=gcc

# Configure additional compiler and linker flags:
CFLAGS+=-std=c99 -Wall -Wextra -I./src -I./tests
LDFLAGS+=

# If you want to pass some MACROS to your code you can use the following 
# variable just add your -D<MY_MACRO>:
CODE_MACROS+=

# Configure Library name:
LIBNAME=zvector
# Configure desired directory where to store the compiled library:
LIBDIR=lib

# Configure Library source directory and temporary object directory:
SRC=src
OBJ=o

# Configure Library build scripts dir (scripts required to build the library)
SCRIPTSDIR=scripts

# Configure directory containing source Unit Test Files and Integration Test 
# files and configure desired directory where to store compiled tests ready 
# for execution:
TESTDIR=tests
TESTBIN=$(TESTDIR)/bin
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
# 				  or IoT applications where you won't have Linux and the 
#				  glibc available. Otherwise you should stick to 0.
MEMX_METHOD=0

# Do you want the library to be built to be thread safe? (and so it uses mutex 
# etc)? If so, set the following variable to 1 to enable thread safe code or 
# set it to 0 to disable the thread safe code within the library:
# Recommendation: If you need to squeeze as much performance as possible out
# 				  of ZVector and thread safety is not a requirement for you
#				  then disable this option (set it to 0 zero). It will make
#				  ZVector gain some extra performance.
THREAD_SAFE_BUILD=1

# Do you want ZVector code to be fully reentrant?
# 0 for no full reentrant code
# 1 for yes full reentrant code
FULL_REENTRANT=1

# Do you want the DMF (Data Manipulation Functions) extensions enabled?
# This extension enables functions like vect_swap that allows you to swap
# two elements of the same vector.
DMF_EXTENSIONS=1

# Do you want the SFMD (Single Function Multiple Data) extension enabled?
# This extension provides ZVect functions that you can call to modify entire
# vectors using a single function call.
# If you want the SFMD extensions enabled the set the following variable to 1
# otherwise set it to 0
SFMD_EXTENSIONS=1

#
###############################################################################

###############################################################################
# Automated part of th Makefile:

RVAL0 =

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
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CCFLAGS += -D LINUX
		RVAL0 = chmod -fR +x $(WDIR)/scripts/
    endif
    ifeq ($(UNAME_S),Darwin)
        CCFLAGS += -D OSX
		RVAL0 = chmod -fR +x $(WDIR)/scripts/
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

SRCF=$(wildcard $(SRC)/*.c)
OSRCF=$(sort $(SRCF))
OBJF=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(OSRCF))

TESTSLIST=$(wildcard $(TESTDIR)/*.c)
OTESTSLIST=$(sort $(TESTSLIST))
TESTSRCS=$(patsubst $(TESTDIR)%.c, %, $(OTESTSLIST))
#$(info "$(TESTSRCS)")

TESTBINS=$(patsubst %.c, %, $(TESTSRCS))
#$(info "$(TESTBINS)")

LIBST=$(LIBDIR)/lib$(LIBNAME).a
#
###############################################################################

###############################################################################
# Targets:

.PHONY: all
all: CFLAGS+=-O3
all: core test tests

clean:
	$(RM) -r $(LIBDIR) $(OBJ) $(TESTDIR)/bin ./*.o

configure: $(SCRIPTSDIR)/ux_set_extension $(SRC)/$(LIBNAME)_config.h
	@echo ----------------------------------------------------------------
	$(shell "$(RVAL0)")
	$(RVAL1)
	$(RVAL2)
	$(RVAL3)
	$(RVAL4)
	$(RVAL5)
	@echo ----------------------------------------------------------------

core: configure $(LIBDIR) $(LIBST)

test: $(TESTDIR)/bin

tests: $(TESTBINS)
	$(info   )
	$(info ===========================)
	$(info Running all found tests... )
	$(info ===========================)
	for test in $(TESTBINS) ; do ./$(TESTBIN)$$test ; done 

debug: CFLAGS+= -ggdb3
debug: CODE_MACROS+= -DDEBUG
debug: core test tests

$(OBJF): $(OSRCF)
	$(info  )
	$(info ===========================)
	$(info Building $@                )
	$(info ===========================)
	$(CC) -c -o $@ $< $(CFLAGS) $(CODE_MACROS)

$(LIBST): $(OBJ) $(OBJF)
	$(info  )
	$(info ===========================)
	$(info Building $(LIBNAME) library)
	$(info ===========================)
	ar rcs $@ -o $(OBJF)

$(TESTBINS): $(TESTSRCS)
	$(info  )
	$(info ===========================)
	$(info Building test: $@          )
	$(info ===========================)
	$(CC) $(CFLAGS) $(CODE_MACROS) $(TESTDIR)$@.c -I$(WDIR)/src -L$(WDIR)/$(LIBDIR) -l$(LIBNAME) $(LDFLAGS)  -o $(TESTBIN)$@

$(LIBDIR):
	[ ! -d $@ ] && mkdir -p $@

$(OBJ):
	[ ! -d $@ ] && mkdir -p $@

$(TESTDIR)/bin:
	$(info  )
	[ ! -d $@ ] && mkdir -p $@

#
###############################################################################
