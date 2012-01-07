#include <check.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "forchess/ai.h"
#include "forchess/board.h"

#define TEST_AI_TIMEOUT 0
#define TEST_AI_THREADS 1
/* basic AI test */
START_TEST (test_ai_next_move1)
{
	fc_board_t board;
	fc_board_init(&board);
	fc_player_t dummy;
	fc_board_setup(&board, "test/boards/test_ai_next_move.1", &dummy);
	fc_move_t move;
	fc_ai_t ai;
	fc_ai_init(&ai, &board);
	fc_ai_next_move(&ai, &move, NULL, FC_FIRST, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.move == fc_uint64("c8-c1"));
	//printf("1: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&ai, &move, NULL, FC_FOURTH, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.move == fc_uint64("a8-c7"));
	//printf("4: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&ai, &move, NULL, FC_FIRST, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.move == fc_uint64("c1-h1"));
}
END_TEST

/* check the remove capabilities */
START_TEST (test_ai_next_move2)
{
	fc_board_t board;
	fc_board_init(&board);
	fc_player_t dummy;
	fc_board_setup(&board, "test/boards/test_ai_next_move.2", &dummy);
	fc_move_t move;
	fc_ai_t ai;
	fc_ai_init(&ai, &board);
	fc_ai_next_move(&ai, &move, NULL, FC_FIRST, 6, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.piece == FC_KNIGHT);

	fc_board_init(&board);
	fc_board_setup(&board, "test/boards/test_ai_next_move.3", &dummy);
	fc_ai_next_move(&ai, &move, NULL, FC_FIRST, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.piece == FC_PAWN);
}
END_TEST

#define TEST_TIMEOUT_SECS 4
START_TEST (test_ai_timeout)
{
	printf("    Running test_ai_timeout; this will take a few seconds...");
	fflush(stdout);
	fc_board_t board;
	fc_board_init(&board);
	fc_player_t dummy;
	fc_board_setup(&board, "test/boards/test_ai_timeout.1", &dummy);
	fc_move_t move;
	fc_ai_t ai;
	fc_ai_init(&ai, &board);
	time_t start = time(NULL);
	fc_ai_next_move(&ai, &move, NULL, FC_FIRST, 12, TEST_TIMEOUT_SECS, TEST_AI_THREADS);
	time_t finish = time(NULL);
	fail_unless(finish - start <= TEST_TIMEOUT_SECS);
	printf("done.\n");
	fflush(stdout);
}
END_TEST

#undef TEST_AI_THREADS
#define TEST_AI_THREADS 4
/* copied from test_ai_next_move1 and test_ai_next_move2 */
START_TEST (test_ai_threaded_next_move)
{
	fc_board_t board;
	fc_board_init(&board);
	fc_player_t dummy;
	fc_board_setup(&board, "test/boards/test_ai_next_move.1", &dummy);
	fc_move_t move;
	fc_ai_t ai;
	fc_ai_init(&ai, &board);
	fc_ai_next_move(&ai, &move, NULL, FC_FIRST, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.move == fc_uint64("c8-c1"));
	//printf("1: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&ai, &move, NULL, FC_FOURTH, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.move == fc_uint64("a8-c7") ||
			move.move == fc_uint64("a8-b6"));
	//printf("4: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&ai, &move, NULL, FC_FIRST, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	//printf("1: %d, 0x%llx\n", move.piece, move.move);
	fail_unless(move.move == fc_uint64("c1-h1"));

	/* check remove */
	fc_board_init(&board);
	fc_board_setup(&board, "test/boards/test_ai_next_move.2", &dummy);
	fc_ai_init(&ai, &board);
	fc_ai_next_move(&ai, &move, NULL, FC_FIRST, 6, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.piece == FC_KNIGHT);

	fc_board_init(&board);
	fc_board_setup(&board, "test/boards/test_ai_next_move.3", &dummy);
	fc_ai_next_move(&ai, &move, NULL, FC_FIRST, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.piece == FC_PAWN);
}
END_TEST

Suite *ai_suite (void)
{
	Suite *s = suite_create("AI");
	TCase *tc_ai = tcase_create("Core");
	tcase_add_test(tc_ai, test_ai_next_move1);
	tcase_add_test(tc_ai, test_ai_next_move2);
	tcase_add_test(tc_ai, test_ai_timeout);
	tcase_add_test(tc_ai, test_ai_threaded_next_move);
	tcase_set_timeout(tc_ai, 8);
	suite_add_tcase(s, tc_ai);
	return s;
}
