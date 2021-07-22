CC=gcc

LIBNAME=vector

SRC=src
OBJ=o

TEST=tests
TESTBIN=$(TEST)/bin

SRCF=$(wildcard $(SRC)/*.c)
OBJF=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCF))

TESTSLIST=$(wildcard $(TEST)/*.c)
TESTS=$(patsubst $(TEST)%.c, %, $(TESTSLIST))
$(info "$(TESTS)")

TESTBINS=$(patsubst %.c, %, $(TESTS))
$(info "$(TESTBINS)")

LIBDIR=lib
LIB=$(LIBDIR)/lib$(LIBNAME).a

CFLAGS+=-Wall
LDFLAGS+=

.PHONY: all
all: core test

clean:
	$(RM) -r $(LIBDIR) $(OBJ) $(TEST)/bin ./*.o

core: $(LIBDIR) $(LIB)

test: $(TEST)/bin $(TESTBINS)
	for test in $(TESTBINS) ; do ./$(TESTBIN)$$test ; done 

debug: CFLAGS+=-ggdb3
debug: core test

$(OBJF): $(SRCF)
	$(info Building $(OBJF))
	$(CC) -c -o $@ $< $(CFLAGS)
	$(info done)

$(LIB): $(OBJ) $(OBJF) 
	ar rcs $@ -o $(OBJF)


$(TESTBINS): $(TESTS)
	$(CC) $(CFLAGS) $(TEST)$@.c -std=c99 -I`pwd`/src -L`pwd`/lib -lvector -o $(TESTBIN)$@

$(LIBDIR):
	mkdir -p $@

$(OBJ):
	mkdir -p $@

$(TEST)/bin:
	mkdir -p $@