CC=gcc
DBG_FLAGS=-g -O0 -std=c99
CFLAGS=-DNDEBUG -O3 -std=c99
INCLUDES=-I./src
TEST_FILES=test/check_forchess.c test/check_moves.c test/check_board.c test/check_check.c test/check_ai.c
INC_FILES=src/forchess/moves.h src/forchess/board.h src/forchess/ai.h
SRC_FILES=src/ai.c src/board.c src/check.c src/moves.c
EXAMPLE_FILES=examples/cli/simple.c

check: $(TEST_FILES) $(INC_FILES) $(SRC_FILES)
	$(CC) -o test_all $(DBG_FLAGS) $(INCLUDES) $(TEST_FILES) $(SRC_FILES) -lcheck
	./test_all

examples: $(EXAMPLE_FILES) $(INC_FILES) $(SRC_FILES)
	$(CC) $(CFLAGS) $(INCLUDES) $(EXAMPLE_FILES) $(SRC_FILES)
