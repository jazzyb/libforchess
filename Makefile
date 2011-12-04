CC=gcc

ifeq ($(DEBUG), 1)
CFLAGS=-g -O0
else
CFLAGS=-DNDEBUG -O3
endif

WARN_FLAGS=-Wall -Werror -std=c89 -pedantic
PROF_FLAGS=-pg

INCLUDES=-I./include

LIBS=-Llib

# Include MacPorts directories for the check library.
ifeq (darwin, $(findstring darwin,$(OSTYPE)))
CHECK_INC=-I/opt/local/include
CHECK_LIB=-L/opt/local/lib
endif
CHECK_FLAGS=$(CHECK_INC) $(CHECK_LIB)

TEST_FILES=test/check_forchess.c \
	   test/check_moves.c \
	   test/check_board.c \
	   test/check_check.c \
	   test/check_fifo.c \
	   test/check_threads.c \
	   test/check_ai.c \
	   test/check_game.c

INC_FILES=include/forchess/moves.h \
	  include/forchess/board.h \
	  include/forchess/fifo.h \
	  include/forchess/threads.h \
	  include/forchess/ai.h \
	  include/forchess/game.h

SRC_FILES=src/ai.c \
	  src/board.c \
	  src/check.c \
	  src/fifo.c \
	  src/game.c \
	  src/moves.c \
	  src/threads.c

OBJ_FILES=src/ai.o \
	  src/board.o \
	  src/check.o \
	  src/fifo.o \
	  src/game.o \
	  src/moves.o \
	  src/threads.o

EXAMPLE_FILES=examples/cli/simple.c


%.o: %.c $(INC_FILES)
	$(CC) -c -o $@ $(CFLAGS) $(WARN_FLAGS) $(INCLUDES) $<

# static or shared library?
ifeq ($(STATIC), 1)
libforchess: $(OBJ_FILES)
	mkdir -p lib
	ar cr lib/libforchess.a $^
	ranlib lib/libforchess.a
else
libforchess: $(OBJ_FILES)
	mkdir -p lib
	$(CC) -shared -o lib/libforchess.so $^
endif

# FIXME: C99 standard just makes compiling easier; will need to change this
# later; see also examples and profiler
check: $(TEST_FILES) libforchess
	$(CC) -o test_all $(CFLAGS) --std=c99 $(INCLUDES) $(CHECK_FLAGS) $(LIBS) $(TEST_FILES) -lcheck -lforchess
	./test_all

examples: $(EXAMPLE_FILES) $(INC_FILES) libforchess
	$(CC) $(CFLAGS) --std=c99 $(INCLUDES) $(LIBS) $(EXAMPLE_FILES) -lforchess

cscope:
	find src -type f | egrep '.*\.h|.*\.c$$' > cscope.files
	cscope -b -i cscope.files

# Even though the header files are the dependencies of generating the
# documentation, sometimes make thinks they are up to date when they aren't.
# In that case we always want to force doxygen to run.
docs: $(INC_FILES) force
	doxygen docs/Doxyfile

all: libforchess check examples cscope docs

# Run the gprof profiler.
libforchess_gprof: $(SRC_FILES) $(INC_FILES)
	$(CC) -c -o src/ai.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/ai.c
	$(CC) -c -o src/board.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/board.c
	$(CC) -c -o src/check.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/check.c
	$(CC) -c -o src/fifo.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/fifo.c
	$(CC) -c -o src/moves.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/moves.c
	$(CC) -c -o src/threads.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/threads.c
	$(CC) -c -o src/game.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/game.c
	mkdir -p lib
	ar cr lib/libforchess.a src/*.o
	ranlib lib/libforchess.a

profiler: $(EXAMPLE_FILES) $(INC_FILES) libforchess_gprof
	$(CC) $(CFLAGS) --std=c99 $(PROF_FLAGS) $(INCLUDES) $(LIBS) $(EXAMPLE_FILES) -lforchess
	./a.out
	gprof ./a.out > gprof.output

clean:
	rm -rf test_all a.out src/*.o lib/ *.dSYM docs/man/ gmon.out gprof.output cscope.files cscope.out

force: ;
