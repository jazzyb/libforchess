CC=gcc
CFLAGS=-g -O0 -std=c99
check: test/check_forchess.c test/check_moves.c src/moves.c src/forchess/moves.h test/check_board.c src/board.c src/forchess/board.h src/check.c test/check_check.c test/check_ai.c src/ai.c src/forchess/ai.h
	$(CC) $(CFLAGS) -I./src test/check_forchess.c test/check_moves.c src/moves.c test/check_board.c src/board.c src/check.c test/check_check.c src/ai.c test/check_ai.c -lcheck
