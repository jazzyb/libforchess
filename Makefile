CC=gcc
CFLAGS=-g -O0 -std=c99
OPT_CFLAGS=-g -O2 -std=c99
TEST_FILES=test/check_forchess.c test/check_moves.c test/check_board.c test/check_check.c test/check_ai.c
INC_FILES=src/forchess/moves.h src/forchess/board.h src/forchess/ai.h
SRC_FILES=src/ai.c src/board.c src/check.c src/moves.c
EXAMPLE_FILES=examples/cli/simple.c

check: $(TEST_FILES) $(INC_FILES) $(SRC_FILES)
	$(CC) $(CFLAGS) -I./src $(TEST_FILES) $(SRC_FILES) -lcheck

examples: $(EXAMPLE_FILES) $(INC_FILES) $(SRC_FILES)
	$(CC) $(OPT_CFLAGS) -I./src $(EXAMPLE_FILES) $(SRC_FILES)
