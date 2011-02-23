CC=gcc
CFLAGS=-g -O0 -std=c99
check: test/check_forchess.c src/forchess.c src/forchess.h
	$(CC) $(CFLAGS) -I./src test/check_forchess.c src/forchess.c -lcheck
