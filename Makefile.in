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

TEST_FILES=test/check_forchess.c \
	   test/check_moves.c \
	   test/check_board.c \
	   test/check_check.c \
	   test/check_ai.c \

INC_FILES=include/forchess/moves.h \
	  include/forchess/board.h \
	  include/forchess/ai.h

SRC_FILES=src/ai.c \
	  src/board.c \
	  src/check.c \
	  src/moves.c

OBJ_FILES=src/ai.o \
	  src/board.o \
	  src/check.o \
	  src/moves.o

EXAMPLE_FILES=example/simple.c example/game.c


%.o: %.c $(INC_FILES)
	$(CC) -c -o $@ $(CFLAGS) $(WARN_FLAGS) $(INCLUDES) $<

libforchess: $(OBJ_FILES)
	mkdir -p lib
	$(CC) -shared -o lib/libforchess.so $^

# FIXME: C99 standard just makes compiling easier; will need to change this
# later; see also example and profiler
check: $(TEST_FILES) libforchess
	$(CC) -o test_all $(CFLAGS) --std=c99 $(INCLUDES) $(LIBS) $(TEST_FILES) -lcheck -lforchess
	./test_all

example: $(EXAMPLE_FILES) $(INC_FILES) libforchess
	$(CC) $(CFLAGS) --std=c99 $(INCLUDES) $(LIBS) $(EXAMPLE_FILES) -lforchess

all: libforchess check example

# Run the gprof profiler.
libforchess_gprof: $(SRC_FILES) $(INC_FILES)
	$(CC) -c -o src/ai.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/ai.c
	$(CC) -c -o src/board.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/board.c
	$(CC) -c -o src/check.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/check.c
	$(CC) -c -o src/moves.o $(CFLAGS) $(WARN_FLAGS) $(PROF_FLAGS) $(INCLUDES) src/moves.c
	mkdir -p lib
	ar cr lib/libforchess.a src/*.o
	ranlib lib/libforchess.a

profiler: $(EXAMPLE_FILES) $(INC_FILES) libforchess_gprof
	$(CC) $(CFLAGS) --std=c99 $(PROF_FLAGS) $(INCLUDES) $(LIBS) $(EXAMPLE_FILES) -lforchess
	./a.out
	gprof ./a.out > gprof.output

clean:
	rm -rf test_all a.out src/*.o lib/ *.dSYM docs/man/ gmon.out gprof.output cscope.files cscope.out
