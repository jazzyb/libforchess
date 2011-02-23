#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "forchess.h"

START_TEST (test_forchess_getters_and_setters)
{
	fc_board_t board;
	bzero(board, sizeof(board));
	fc_player_t player = -1;
	fc_piece_t piece = -1;
	int ret = fc_get_piece(&board, &player, &piece, 3, 3);
	fail_unless(ret == 0);
	ret = fc_set_piece(&board, FC_FOURTH, FC_QUEEN, 3, 3);
	fail_unless(ret == 1);
	ret = fc_get_piece(&board, &player, &piece, 3, 3);
	fail_unless(ret == 1 && player == FC_FOURTH && piece == FC_QUEEN);
}
END_TEST

START_TEST (test_forchess_board_setup)
{
	fc_board_t board;
	bzero(board, sizeof(board));

	/* check bad boards */
	int ret = fc_setup_board(&board,
			         "test/boards/test_forchess_board_setup.1");
	fail_unless(ret == 0);
	bzero(board, sizeof(board));
	ret = fc_setup_board(&board,
			     "test/boards/test_forchess_board_setup.2");
	fail_unless(ret == 0);
	bzero(board, sizeof(board));
	ret = fc_setup_board(&board,
			     "test/boards/test_forchess_board_setup.3");
	fail_unless(ret == 0);
	bzero(board, sizeof(board));
	ret = fc_setup_board(&board,
			     "test/boards/test_forchess_board_setup.4");
	fail_unless(ret == 0);

	/* now check that the pieces were placed in the appropriate spots */
	bzero(board, sizeof(board));
	ret = fc_setup_board(&board,
			     "test/boards/test_forchess_board_setup.5");
	fail_unless(ret == 1);
	fc_player_t player;
	fc_piece_t piece;
	ret = fc_get_piece(&board, &player, &piece, 2, 2);
	fail_unless(ret == 1 && player == FC_FIRST && piece == FC_PAWN);
	ret = fc_get_piece(&board, &player, &piece, 6, 7);
	fail_unless(ret == 1 && player == FC_SECOND && piece == FC_KING);
	ret = fc_get_piece(&board, &player, &piece, 4, 3);
	fail_unless(ret == 1 && player == FC_THIRD && piece == FC_QUEEN);
	ret = fc_get_piece(&board, &player, &piece, 3, 4);
	fail_unless(ret == 1 && player == FC_FOURTH && piece == FC_BISHOP);
}
END_TEST

extern int fc_get_king_moves (fc_board_t *board,
			      fc_move_t **moves,
			      int *moves_len,
			      fc_player_t player);
START_TEST (test_forchess_king_moves)
{
	fc_board_t board;
	bzero(board, sizeof(board));

	fc_set_piece(&board, FC_FIRST, FC_KING, 0, 0);
	fc_set_piece(&board, FC_SECOND, FC_PAWN, 2, 2);
	fc_set_piece(&board, FC_SECOND, FC_KING, 3, 3);
	fc_set_piece(&board, FC_THIRD, FC_KING, 7, 7);
	fc_set_piece(&board, FC_FOURTH, FC_PAWN, 4, 4);

	int len;
	fc_move_t *moves = NULL;
	int ret = fc_get_king_moves(&board, &moves, &len, FC_FIRST);
	fail_unless(ret == 1 && len == 8 && moves);
	fail_unless(moves[0].move == UINT64_C(0x101));
	fail_unless(moves[1].move == UINT64_C(0x201));
	fail_unless(moves[2].move == UINT64_C(0x3));
	fail_unless(moves[3].move == UINT64_C(0x0));

	ret = fc_get_king_moves(&board, &moves, &len, FC_THIRD);
	fail_unless(ret == 1 && len == 8 && moves);
	fail_unless(moves[0].move == UINT64_C(0xc000000000000000));
	fail_unless(moves[1].move == UINT64_C(0x8040000000000000));
	fail_unless(moves[2].move == UINT64_C(0x8080000000000000));
	fail_unless(moves[3].move == UINT64_C(0x0));

	ret = fc_get_king_moves(&board, &moves, &len, FC_SECOND);
	fail_unless(ret == 1 && len == 8 && moves);
	fail_unless(moves[0].move == UINT64_C(0x0408000000));
	fail_unless(moves[1].move == UINT64_C(0x000c000000));
	fail_unless(moves[2].move == UINT64_C(0x0808000000));
	fail_unless(moves[3].move == UINT64_C(0x0008080000));
	fail_unless(moves[4].move == UINT64_C(0x1008000000));
	fail_unless(moves[5].move == UINT64_C(0x0018000000));
	fail_unless(moves[6].move == UINT64_C(0x0008100000));
	fail_unless(moves[7].move == UINT64_C(0x0));

	ret = fc_get_king_moves(&board, &moves, &len, FC_FOURTH);
	fail_unless(ret == 0);
	free(moves);
}
END_TEST

Suite *forchess_suite (void)
{
	Suite *s = suite_create("Forchess");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, test_forchess_getters_and_setters);
	tcase_add_test(tc_core, test_forchess_board_setup);
	tcase_add_test(tc_core, test_forchess_king_moves);
	suite_add_tcase(s, tc_core);
	return s;
}

int main (int argc, char **argv)
{
	int number_failed;
	Suite *s = forchess_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}
