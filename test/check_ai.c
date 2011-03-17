#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "forchess/ai.h"
#include "forchess/board.h"

START_TEST (test_ai_score_position)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_board_setup(&board, "test/boards/test_ai_score_position.1");
	fail_unless(fc_ai_score_position(&board, FC_FIRST) == -100);
}
END_TEST

START_TEST (test_ai_is_move_valid)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_board_setup(&board, "test/boards/test_ai_is_move_valid.1");
	fc_move_t move;
	/* test moving king into check */
	move.player = FC_FIRST;
	move.piece = FC_QUEEN;
	move.move = fc_uint64("b1-c2");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 0);
	/* test putting partner's king into check */
	move.player = FC_FIRST;
	move.piece = FC_BISHOP;
	move.move = fc_uint64("h7-g6");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 0);
	/* forcing us to move out of check */
	move.player = FC_FOURTH;
	move.piece = FC_ROOK;
	move.move = fc_uint64("g1-f1");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 0);
	move.player = FC_FOURTH;
	move.piece = FC_ROOK;
	move.move = fc_uint64("h2-g2");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 1);
	/* if checkmate don't move the king, but move anything else */
	move.player = FC_SECOND;
	move.piece = FC_KING;
	move.move = fc_uint64("a8-b8");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 0);
	move.player = FC_SECOND;
	move.piece = FC_KNIGHT;
	move.move = fc_uint64("a7-c8");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 1);
	/* check capture king to get out of check */
	fc_board_remove_piece(&board, 6, 7);
	move.player = FC_THIRD;
	move.piece = FC_BISHOP;
	move.move = fc_uint64("d5-h1");
	fail_unless(fc_ai_is_move_valid(&board, &move) == 1);
	/* TODO test a remove which puts the king into check(mate) */
}
END_TEST

START_TEST (test_ai_next_move)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_board_setup(&board, "test/boards/test_ai_next_move.1");
	fc_move_t move;
	fc_ai_next_move(&board, &move, FC_FIRST, 6);
	printf("1: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_mlist_t list;
	fc_mlist_init(&list, 0);
	fc_board_get_moves(&board, &list, FC_SECOND);
	printf("# moves == %d\n", fc_mlist_length(&list));
	for (int i = 0; i < fc_mlist_length(&list); i++) {
		fc_board_t copy;
		fc_board_copy(&copy, &board);
		fc_board_make_move(&copy, fc_mlist_get(&list, i));
		printf("%d %d\n", i, fc_is_king_in_check(&copy, FC_SECOND));
	}
	fc_ai_next_move(&board, &move, FC_SECOND, 6);
	printf("2: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_FOURTH, 6);
	printf("4: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_FIRST, 6);
	printf("1: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_SECOND, 6);
	printf("2: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_FOURTH, 6);
	printf("4: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_FIRST, 6);
	printf("1: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_SECOND, 6);
	printf("2: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_FOURTH, 6);
	printf("4: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_FIRST, 6);
	printf("1: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_SECOND, 6);
	printf("2: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	fc_ai_next_move(&board, &move, FC_FOURTH, 6);
	printf("4: %d, 0x%llx\n", move.piece, move.move);
	fc_board_make_move(&board, &move);
	//fail_unless(move.move == fc_uint64("b8-h8"));
#if 0
	fc_board_make_move(&board, &move);
	bzero(&move, sizeof(move));
	fc_mlist_t list;
	fc_mlist_init(&list, 0);
	fc_board_get_moves(&board, &list, FC_SECOND);
	printf("# moves == %d\n", fc_mlist_length(&list));
	for (int i = 0; i < fc_mlist_length(&list); i++) {
		fc_board_t copy;
		fc_board_copy(&copy, &board);
		fc_board_make_move(&copy, fc_mlist_get(&list, i));
		printf("%d %d\n", i, fc_is_king_in_check(&copy, FC_SECOND));
	}
	fc_ai_next_move(&board, &move, FC_SECOND, 3);
	printf("0x%llx\n0x%llx\n", move.move, fc_uint64("d5-e4"));
#endif
	//fail_unless(move.move == fc_uint64("d5-e4"));
	//fc_board_make_move(&board, &move);
}
END_TEST

Suite *ai_suite (void)
{
	Suite *s = suite_create("AI");
	TCase *tc_ai = tcase_create("Core");
	tcase_add_test(tc_ai, test_ai_score_position);
	tcase_add_test(tc_ai, test_ai_is_move_valid);
	tcase_add_test(tc_ai, test_ai_next_move);
	suite_add_tcase(s, tc_ai);
	return s;
}
