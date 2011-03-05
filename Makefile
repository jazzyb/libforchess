CC=gcc
CFLAGS=-g -O0 -std=c99
check: test/check_forchess.c test/check_moves.c src/moves.c src/forchess/moves.h test/check_board.c src/board.c src/forchess/board.h
	$(CC) $(CFLAGS) -I./src test/check_forchess.c test/check_moves.c src/moves.c test/check_board.c src/board.c -lcheck
