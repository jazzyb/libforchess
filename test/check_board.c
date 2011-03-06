#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "forchess/board.h"

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

extern void fc_get_king_moves (fc_board_t *board,
			      fc_mlist_t *moves,
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
	fc_mlist_t moves;
	int ret = fc_mlist_init(&moves, 0);
	fc_get_king_moves(&board, &moves, FC_FIRST);
	fail_unless(fc_mlist_length(&moves) == 3);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("a1-a2"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("a1-b2"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("a1-b1"));

	fc_mlist_clear(&moves);
	fc_get_king_moves(&board, &moves, FC_THIRD);
	fail_unless(fc_mlist_length(&moves) == 3);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("h8-g8"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("h8-g7"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("h8-h7"));

	fc_mlist_clear(&moves);
	fc_get_king_moves(&board, &moves, FC_SECOND);
	fail_unless(fc_mlist_length(&moves) == 6);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("d4-c5"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("d4-c4"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("d4-d5"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("d4-d3"));
	fail_unless(fc_mlist_get(&moves, 4)->move == fc_uint64("d4-e4"));
	fail_unless(fc_mlist_get(&moves, 5)->move == fc_uint64("d4-e3"));

	fc_mlist_clear(&moves);
	fc_get_king_moves(&board, &moves, FC_FOURTH);
	fail_unless(moves.index == 0);
}
END_TEST

extern void fc_get_knight_moves (fc_board_t *board,
				 fc_mlist_t *moves,
				 fc_piece_t piece);

START_TEST (test_forchess_knight_moves)
{
	fc_board_t board;
	bzero(board, sizeof(board));

	fc_set_piece(&board, FC_FIRST, FC_KNIGHT, 2, 2);
	fc_set_piece(&board, FC_SECOND, FC_KNIGHT, 1, 0);
	fc_set_piece(&board, FC_THIRD, FC_KNIGHT, 0, 1);
	fc_set_piece(&board, FC_FOURTH, FC_KNIGHT, 7, 7);
	fc_set_piece(&board, FC_FOURTH, FC_KNIGHT, 5, 6);

	fc_mlist_t moves;
	fc_mlist_init(&moves, 0);
	fc_get_knight_moves(&board, &moves, FC_FIRST);
	fail_unless(fc_mlist_length(&moves) == 7);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("c3-a4"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("c3-a2"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("c3-b5"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("c3-e4"));
	fail_unless(fc_mlist_get(&moves, 4)->move == fc_uint64("c3-e2"));
	fail_unless(fc_mlist_get(&moves, 5)->move == fc_uint64("c3-d5"));
	fail_unless(fc_mlist_get(&moves, 6)->move == fc_uint64("c3-d1"));

	fc_mlist_clear(&moves);
	fc_get_knight_moves(&board, &moves, FC_SECOND);
	fail_unless(fc_mlist_length(&moves) == 3);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("a2-c3"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("a2-c1"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("a2-b4"));

	fc_mlist_clear(&moves);
	fc_get_knight_moves(&board, &moves, FC_THIRD);
	fail_unless(fc_mlist_length(&moves) == 2);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("b1-a3"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("b1-d2"));

	fc_mlist_clear(&moves);
	fc_get_knight_moves(&board, &moves, FC_FOURTH);
	fail_unless(fc_mlist_length(&moves) == 6);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("g6-e7"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("g6-e5"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("g6-f8"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("g6-f4"));
	fail_unless(fc_mlist_get(&moves, 4)->move == fc_uint64("g6-h4"));
	fail_unless(fc_mlist_get(&moves, 5)->move == fc_uint64("h8-f7"));
}
END_TEST

extern void fc_get_pawn_moves (fc_board_t *board,
			       fc_mlist_t *moves,
			       fc_player_t player);

START_TEST (test_forchess_pawn_moves)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_set_piece(&board, FC_FIRST, FC_PAWN, 1, 3);
	fc_set_piece(&board, FC_FIRST, FC_PAWN, 3, 3);
	fc_set_piece(&board, FC_SECOND, FC_PAWN, 1, 4);
	fc_set_piece(&board, FC_SECOND, FC_PAWN, 4, 3);
	fc_set_piece(&board, FC_THIRD, FC_PAWN, 4, 4);
	fc_set_piece(&board, FC_THIRD, FC_PAWN, 5, 3);
	fc_set_piece(&board, FC_FOURTH, FC_PAWN, 0, 3);
	fc_set_piece(&board, FC_FOURTH, FC_PAWN, 5, 4);

	fc_mlist_t moves;
	fc_mlist_init(&moves, 0);
	fc_get_pawn_moves(&board, &moves, FC_FIRST);
	fail_unless(fc_mlist_length(&moves) == 3);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("d2-e3"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("d2-e2"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("d4-d5"));

	fc_mlist_clear(&moves);
	fc_get_pawn_moves(&board, &moves, FC_SECOND);
	fail_unless(fc_mlist_length(&moves) == 4);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("e2-f1"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("d5-e4"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("d5-d4"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("d5-e5"));

	fc_mlist_clear(&moves);
	fc_get_pawn_moves(&board, &moves, FC_THIRD);
	fail_unless(fc_mlist_length(&moves) == 3);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("e5-d5"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("d6-c5"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("d6-d5"));

	fc_mlist_clear(&moves);
	fc_get_pawn_moves(&board, &moves, FC_FOURTH);
	fail_unless(fc_mlist_length(&moves) == 4);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("d1-c2"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("d1-d2"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("e6-d7"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("e6-d6"));
}
END_TEST

START_TEST (test_forchess_bishop_moves)
{
	/* TODO */
}
END_TEST

Suite *board_suite (void)
{
	Suite *s = suite_create("Board");
	TCase *tc_board = tcase_create("Core");
	tcase_add_test(tc_board, test_forchess_getters_and_setters);
	tcase_add_test(tc_board, test_forchess_board_setup);
	tcase_add_test(tc_board, test_forchess_king_moves);
	tcase_add_test(tc_board, test_forchess_knight_moves);
	tcase_add_test(tc_board, test_forchess_pawn_moves);
	//tcase_add_test(tc_board, test_forchess_bishop_moves);
	suite_add_tcase(s, tc_board);
	return s;
}

