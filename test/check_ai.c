#include <check.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "forchess/ai.h"
#include "forchess/board.h"

START_TEST (test_ai_score_position)
{
	fc_board_t board;
	fc_board_init(&board);
	fc_player_t dummy;
	fc_board_setup(&board, "test/boards/test_ai_score_position.1", &dummy);
	fc_ai_t ai;
	fc_ai_init(&ai, &board);
	fail_unless(fc_ai_score_position(&ai, FC_FIRST) == -100);
}
END_TEST

START_TEST (test_ai_is_move_valid)
{
	fc_board_t board;
	fc_board_init(&board);
	fc_player_t dummy;
	fc_board_setup(&board, "test/boards/test_ai_is_move_valid.1", &dummy);
	fc_move_t move;
	/* test moving king into check */
	move.player = FC_FIRST;
	move.piece = FC_QUEEN;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = fc_uint64("b1-c2");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 0);
	/* test putting partner's king into check */
	move.player = FC_FIRST;
	move.piece = FC_BISHOP;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = fc_uint64("h7-g6");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 0);
	/* forcing us to move out of check */
	move.player = FC_FOURTH;
	move.piece = FC_ROOK;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = fc_uint64("g1-f1");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 0);
	move.player = FC_FOURTH;
	move.piece = FC_ROOK;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = fc_uint64("h2-g2");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 1);
	/* if checkmate don't move the king, but move anything else */
	move.player = FC_SECOND;
	move.piece = FC_KING;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = fc_uint64("a8-b8");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 0);
	move.player = FC_SECOND;
	move.piece = FC_KNIGHT;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = fc_uint64("a7-c8");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 1);
	/* check capture king to get out of check */
	fc_board_remove_piece(&board, 6, 7);
	move.player = FC_THIRD;
	move.piece = FC_BISHOP;
	move.opp_player = FC_FOURTH;
	move.opp_piece = FC_KING;
	move.promote = FC_NONE;
	move.move = fc_uint64("d5-h1");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 1);
	/* TODO test a remove which puts the king into check(mate) */
}
END_TEST

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
	fc_ai_next_move(&ai, &move, FC_FIRST, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.move == fc_uint64("c8-c1"));
	//printf("1: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&ai, &move, FC_FOURTH, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.move == fc_uint64("a8-c7"));
	//printf("4: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&ai, &move, FC_FIRST, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
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
	fc_ai_next_move(&ai, &move, FC_FIRST, 6, TEST_AI_TIMEOUT, TEST_AI_THREADS);
	fail_unless(move.piece == FC_KNIGHT);

	fc_board_init(&board);
	fc_board_setup(&board, "test/boards/test_ai_next_move.3", &dummy);
	fc_ai_next_move(&ai, &move, FC_FIRST, 4, TEST_AI_TIMEOUT, TEST_AI_THREADS);
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
	fc_ai_next_move(&ai, &move, FC_FIRST, 12, TEST_TIMEOUT_SECS, TEST_AI_THREADS);
	time_t finish = time(NULL);
	fail_unless(finish - start <= TEST_TIMEOUT_SECS);
	printf("done.\n");
	fflush(stdout);
}
END_TEST

Suite *ai_suite (void)
{
	Suite *s = suite_create("AI");
	TCase *tc_ai = tcase_create("Core");
	tcase_add_test(tc_ai, test_ai_score_position);
	tcase_add_test(tc_ai, test_ai_is_move_valid);
	tcase_add_test(tc_ai, test_ai_next_move1);
	tcase_add_test(tc_ai, test_ai_next_move2);
	tcase_add_test(tc_ai, test_ai_timeout);
	tcase_set_timeout(tc_ai, 8);
	suite_add_tcase(s, tc_ai);
	return s;
}
