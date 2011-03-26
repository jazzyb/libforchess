#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "forchess/board.h"

START_TEST (test_forchess_knight_checks)
{
	fc_board_t board;
	bzero(&board, sizeof(board));

	fc_board_set_piece(&board, FC_FIRST, FC_KING, 0, 0);
	fail_unless(!fc_board_check_status(&board, FC_FIRST));
	fc_board_set_piece(&board, FC_SECOND, FC_KNIGHT, 2, 1);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fc_board_remove_piece(&board, 2, 1);
	fc_board_set_piece(&board, FC_SECOND, FC_KNIGHT, 1, 2);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);

	fc_board_set_piece(&board, FC_SECOND, FC_KING, 7, 0);
	fail_unless(!fc_board_check_status(&board, FC_SECOND));
	fc_board_set_piece(&board, FC_FIRST, FC_KNIGHT, 6, 2);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_remove_piece(&board, 6, 2);
	fc_board_set_piece(&board, FC_FIRST, FC_KNIGHT, 5, 1);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);

	fc_board_set_piece(&board, FC_THIRD, FC_KING, 7, 7);
	fail_unless(!fc_board_check_status(&board, FC_THIRD));
	fc_board_set_piece(&board, FC_FOURTH, FC_KNIGHT, 5, 6);
	fail_unless(fc_board_check_status(&board, FC_THIRD) == FC_CHECK);
	fc_board_remove_piece(&board, 5, 6);
	fc_board_set_piece(&board, FC_FOURTH, FC_KNIGHT, 6, 5);
	fail_unless(fc_board_check_status(&board, FC_THIRD) == FC_CHECK);

	fc_board_set_piece(&board, FC_FOURTH, FC_KING, 0, 7);
	fail_unless(!fc_board_check_status(&board, FC_FOURTH));
	fc_board_set_piece(&board, FC_THIRD, FC_KNIGHT, 1, 5);
	fail_unless(fc_board_check_status(&board, FC_FOURTH) == FC_CHECK);
	fc_board_remove_piece(&board, 1, 5);
	fc_board_set_piece(&board, FC_THIRD, FC_KNIGHT, 2, 6);
	fail_unless(fc_board_check_status(&board, FC_FOURTH) == FC_CHECK);
}
END_TEST

START_TEST (test_forchess_lateral_checks)
{
	fc_board_t board;
	bzero(&board, sizeof(board));

	fc_board_set_piece(&board, FC_FIRST, FC_KING, 2, 2);
	fc_board_set_piece(&board, FC_SECOND, FC_ROOK, 2, 7);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fc_board_remove_piece(&board, 2, 7);
	fc_board_set_piece(&board, FC_SECOND, FC_QUEEN, 7, 2);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);

	fc_board_set_piece(&board, FC_SECOND, FC_KING, 5, 5);
	fc_board_set_piece(&board, FC_FIRST, FC_ROOK, 0, 5);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_remove_piece(&board, 0, 5);
	fc_board_set_piece(&board, FC_FIRST, FC_QUEEN, 5, 0);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
}
END_TEST

START_TEST (test_forchess_diagonal_checks)
{
	fc_board_t board;
	bzero(&board, sizeof(board));

	fc_board_set_piece(&board, FC_FIRST, FC_KING, 1, 3);
	fc_board_set_piece(&board, FC_SECOND, FC_BISHOP, 3, 5);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fc_board_remove_piece(&board, 3, 5);
	fc_board_set_piece(&board, FC_SECOND, FC_QUEEN, 3, 1);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);

	fc_board_set_piece(&board, FC_SECOND, FC_KING, 6, 5);
	fc_board_set_piece(&board, FC_FIRST, FC_BISHOP, 4, 3);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_remove_piece(&board, 4, 3);
	fc_board_set_piece(&board, FC_FIRST, FC_QUEEN, 4, 7);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
}
END_TEST

START_TEST (test_forchess_king_checks)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_board_set_piece(&board, FC_FIRST, FC_KING, 3, 3);
	fc_board_set_piece(&board, FC_SECOND, FC_KING, 4, 4);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_remove_piece(&board, 4, 4);
	fc_board_set_piece(&board, FC_SECOND, FC_KING, 4, 3);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_set_piece(&board, FC_SECOND, FC_KING, 4, 2);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_set_piece(&board, FC_SECOND, FC_KING, 3, 2);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
}
END_TEST

START_TEST (test_forchess_pawn_checks)
{
	fc_board_t board;
	bzero(&board, sizeof(board));

	fc_board_set_piece(&board, FC_FIRST, FC_KING, 1, 1);
	fc_board_set_piece(&board, FC_SECOND, FC_PAWN, 1, 0);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fc_board_remove_piece(&board, 1, 0);
	fc_board_set_piece(&board, FC_SECOND, FC_PAWN, 2, 0);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == 0);
	fc_board_remove_piece(&board, 2, 0);
	fc_board_set_piece(&board, FC_SECOND, FC_PAWN, 2, 1);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fc_board_remove_piece(&board, 2, 1);
	fc_board_set_piece(&board, FC_SECOND, FC_PAWN, 1, 2);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == 0);
	fc_board_remove_piece(&board, 1, 2);
	fc_board_set_piece(&board, FC_FOURTH, FC_PAWN, 1, 2);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fc_board_remove_piece(&board, 1, 2);
	fc_board_set_piece(&board, FC_FOURTH, FC_PAWN, 0, 1);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);

	bzero(&board, sizeof(board));
	fc_board_set_piece(&board, FC_SECOND, FC_KING, 3, 3);
	fc_board_set_piece(&board, FC_FIRST, FC_PAWN, 2, 3);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_remove_piece(&board, 2, 3);
	fc_board_set_piece(&board, FC_FIRST, FC_PAWN, 3, 2);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_remove_piece(&board, 3, 2);
	fc_board_set_piece(&board, FC_THIRD, FC_PAWN, 4, 3);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_remove_piece(&board, 4, 3);
	fc_board_set_piece(&board, FC_THIRD, FC_PAWN, 3, 4);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);

	bzero(&board, sizeof(board));
	fc_board_set_piece(&board, FC_THIRD, FC_KING, 5, 5);
	fc_board_set_piece(&board, FC_SECOND, FC_PAWN, 5, 4);
	fail_unless(fc_board_check_status(&board, FC_THIRD) == FC_CHECK);
	fc_board_remove_piece(&board, 5, 4);
	fc_board_set_piece(&board, FC_SECOND, FC_PAWN, 6, 5);
	fail_unless(fc_board_check_status(&board, FC_THIRD) == FC_CHECK);
	fc_board_remove_piece(&board, 6, 5);
	fc_board_set_piece(&board, FC_FOURTH, FC_PAWN, 5, 6);
	fail_unless(fc_board_check_status(&board, FC_THIRD) == FC_CHECK);
	fc_board_remove_piece(&board, 5, 6);
	fc_board_set_piece(&board, FC_FOURTH, FC_PAWN, 4, 5);
	fail_unless(fc_board_check_status(&board, FC_THIRD) == FC_CHECK);

	bzero(&board, sizeof(board));
	fc_board_set_piece(&board, FC_FOURTH, FC_KING, 3, 3);
	fc_board_set_piece(&board, FC_FIRST, FC_PAWN, 2, 3);
	fail_unless(fc_board_check_status(&board, FC_FOURTH) == FC_CHECK);
	fc_board_remove_piece(&board, 2, 3);
	fc_board_set_piece(&board, FC_FIRST, FC_PAWN, 3, 2);
	fail_unless(fc_board_check_status(&board, FC_FOURTH) == FC_CHECK);
	fc_board_remove_piece(&board, 3, 2);
	fc_board_set_piece(&board, FC_THIRD, FC_PAWN, 4, 3);
	fail_unless(fc_board_check_status(&board, FC_FOURTH) == FC_CHECK);
	fc_board_remove_piece(&board, 4, 3);
	fc_board_set_piece(&board, FC_THIRD, FC_PAWN, 3, 4);
	fail_unless(fc_board_check_status(&board, FC_FOURTH) == FC_CHECK);
}
END_TEST

START_TEST (test_forchess_checkmate)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_player_t dummy;
	fc_board_setup(&board, "test/boards/test_forchess_pawn_checks.1",
			&dummy);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECKMATE);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECK);
	fc_board_set_piece(&board, FC_FIRST, FC_BISHOP, 3, 3);
	fail_unless(fc_board_check_status(&board, FC_FIRST) == FC_CHECK);
	fail_unless(fc_board_check_status(&board, FC_SECOND) == FC_CHECKMATE);
}
END_TEST

Suite *check_suite (void)
{
	Suite *s = suite_create("Check");
	TCase *tc_check = tcase_create("Core");
	tcase_add_test(tc_check, test_forchess_knight_checks);
	tcase_add_test(tc_check, test_forchess_lateral_checks);
	tcase_add_test(tc_check, test_forchess_diagonal_checks);
	tcase_add_test(tc_check, test_forchess_king_checks);
	tcase_add_test(tc_check, test_forchess_pawn_checks);
	tcase_add_test(tc_check, test_forchess_checkmate);
	suite_add_tcase(s, tc_check);
	return s;
}
