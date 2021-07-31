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
CFLAGS+=-std=c99 -Wall -I./src
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

# Do you want the library to be built to be thread safe? (and so it uses mutex 
# etc)? If so, set the following variable to 1 to enable thread safe code or 
# set it to 0 to disable the thread safe code within the library:
THREAD_SAFE_BUILD=1

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

ifeq ($(THREAD_SAFE_BUILD), 1)
LDFLAGS+= -lpthread
#CODE_MACROS+= -DTHREAD_SAFE
RVAL1 = $(shell $(WDIR)/$(SCRIPTSDIR)/ux_set_extension THREAD_SAFE 1)
else
RVAL1 = $(shell $(WDIR)/$(SCRIPTSDIR)/ux_set_extension THREAD_SAFE 0)
endif

ifeq ($(SFMD_EXTENSIONS), 1)
#CODE_MACROS+= -DZVECT_DMF_EXTENSIONS
RVAL2 = $(shell $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_DMF_EXTENSIONS 1)
else
RVAL2 = $(shell $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_DMF_EXTENSIONS 0)
endif

ifeq ($(SFMD_EXTENSIONS), 1)
#CODE_MACROS+= -DZVECT_SFMD_EXTENSIONS
RVAL3 = $(shell $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_SFMD_EXTENSIONS 1)
else
RVAL3 = $(shell $(WDIR)/$(SCRIPTSDIR)/ux_set_extension ZVECT_SFMD_EXTENSIONS 0)
endif

SRCF=$(wildcard $(SRC)/*.c)
OSRCF=$(sort $(SRCF))
OBJF=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(OSRCF))

TESTSLIST=$(wildcard $(TESTDIR)/*.c)
OTESTSLIST=$(sort $(TESTSLIST))
TESTS=$(patsubst $(TESTDIR)%.c, %, $(OTESTSLIST))
#$(info "$(TESTS)")

TESTBINS=$(patsubst %.c, %, $(TESTS))
#$(info "$(TESTBINS)")

LIBST=$(LIBDIR)/lib$(LIBNAME).a
#
###############################################################################

###############################################################################
# Targets:

.PHONY: all
all: CFLAGS+=-O2
all: core test

clean:
	$(RM) -r $(LIBDIR) $(OBJ) $(TESTDIR)/bin ./*.o

configure: $(SCRIPTSDIR)/ux_set_extension $(SRC)/$(LIBNAME)_config.h
	$(info ----------------------------------------------------------------)
	$(info $(RVAL1))
	$(info $(RVAL2))
	$(info $(RVAL3))
	$(info ----------------------------------------------------------------)

core: configure $(LIBDIR) $(LIBST)

test: $(TESTDIR)/bin $(TESTBINS)
	$(info   )
	$(info ===========================)
	$(info Running all found tests... )
	$(info ===========================)
	for test in $(TESTBINS) ; do ./$(TESTBIN)$$test ; done 

debug: CFLAGS+= -ggdb3
debug: CODE_MACROS+= -DDEBUG
debug: core test

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

$(TESTBINS): $(TESTS)
	$(info  )
	$(info ===========================)
	$(info Building all found tests...)
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
