CC=gcc

CFLAGS=-DNDEBUG -O3 -std=c99
DBG_FLAGS=-g -O0 -std=c99 # Use this instead of CFLAGS if you need to debug.
PROF_FLAGS=-pg

INCLUDES=-I./src

LIBS=-Llib

TEST_FILES=test/check_forchess.c \
	   test/check_moves.c \
	   test/check_board.c \
	   test/check_check.c \
	   test/check_ai.c \
	   test/check_game.c

INC_FILES=src/forchess/moves.h \
	  src/forchess/board.h \
	  src/forchess/ai.h \
	  src/forchess/game.h

SRC_FILES=src/ai.c \
	  src/board.c \
	  src/check.c \
	  src/moves.c \
	  src/game.c \
	  src/threats.c

OBJ_FILES=src/ai.o \
	  src/board.o \
	  src/check.o \
	  src/moves.o \
	  src/game.o \
	  src/threats.o

EXAMPLE_FILES=examples/cli/simple.c


%.o: %.c $(INC_FILES)
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDES) $<

libforchess: $(OBJ_FILES)
	mkdir -p lib
	ar cr lib/libforchess.a $^

check: $(TEST_FILES) libforchess
	$(CC) -o test_all $(CFLAGS) $(INCLUDES) $(LIBS) $(TEST_FILES) -lcheck -lforchess
	./test_all

examples: $(EXAMPLE_FILES) $(INC_FILES) libforchess
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) $(EXAMPLE_FILES) -lforchess

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
	$(CC) -c -o src/ai.o $(CFLAGS) $(PROF_FLAGS) $(INCLUDES) src/ai.c
	$(CC) -c -o src/board.o $(CFLAGS) $(PROF_FLAGS) $(INCLUDES) src/board.c
	$(CC) -c -o src/check.o $(CFLAGS) $(PROF_FLAGS) $(INCLUDES) src/check.c
	$(CC) -c -o src/moves.o $(CFLAGS) $(PROF_FLAGS) $(INCLUDES) src/moves.c
	$(CC) -c -o src/game.o $(CFLAGS) $(PROF_FLAGS) $(INCLUDES) src/game.c
	mkdir -p lib
	ar cr lib/libforchess.a src/*.o

profiler: $(EXAMPLE_FILES) $(INC_FILES) libforchess_gprof
	$(CC) $(CFLAGS) $(PROF_FLAGS) $(INCLUDES) $(LIBS) $(EXAMPLE_FILES) -lforchess
	./a.out
	gprof ./a.out > gprof.output

clean:
	rm -rf test_all a.out src/*.o lib/ docs/man/ gmon.out gprof.output cscope.files cscope.out

force: ;
