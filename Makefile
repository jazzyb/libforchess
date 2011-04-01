CC=gcc

CFLAGS=-DNDEBUG -O3 -std=c99
DBG_FLAGS=-g -O0 -std=c99 # Use this instead of CFLAGS if you need to debug.

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
	  src/game.c

OBJ_FILES=src/ai.o \
	  src/board.o \
	  src/check.o \
	  src/moves.o \
	  src/game.o

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

# Even though the header files are the dependencies of generating the
# documentation, sometimes make thinks they are up to date when they aren't.
# In that case we always want to force doxygen to run.
docs: $(INC_FILES) force
	doxygen docs/Doxyfile

all: libforchess check examples docs

clean:
	rm -rf test_all a.out src/*.o lib/ docs/man/

force: ;
