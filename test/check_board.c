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
	int ret = fc_board_get_piece(&board, &player, &piece, 3, 3);
	fail_unless(ret == 0);
	ret = fc_board_set_piece(&board, FC_FOURTH, FC_QUEEN, 3, 3);
	fail_unless(ret == 1);
	ret = fc_board_get_piece(&board, &player, &piece, 3, 3);
	fail_unless(ret == 1 && player == FC_FOURTH && piece == FC_QUEEN);
}
END_TEST

START_TEST (test_forchess_board_setup)
{
	fc_board_t board;
	bzero(board, sizeof(board));

	/* check bad boards */
	int ret = fc_board_setup(&board,
			         "test/boards/test_forchess_board_setup.1");
	fail_unless(ret == 0);
	bzero(board, sizeof(board));
	ret = fc_board_setup(&board,
			     "test/boards/test_forchess_board_setup.2");
	fail_unless(ret == 0);
	bzero(board, sizeof(board));
	ret = fc_board_setup(&board,
			     "test/boards/test_forchess_board_setup.3");
	fail_unless(ret == 0);
	bzero(board, sizeof(board));
	ret = fc_board_setup(&board,
			     "test/boards/test_forchess_board_setup.4");
	fail_unless(ret == 0);

	/* now check that the pieces were placed in the appropriate spots */
	bzero(board, sizeof(board));
	ret = fc_board_setup(&board,
			     "test/boards/test_forchess_board_setup.5");
	fail_unless(ret == 1);
	fc_player_t player;
	fc_piece_t piece;
	ret = fc_board_get_piece(&board, &player, &piece, 2, 2);
	fail_unless(ret == 1 && player == FC_FIRST && piece == FC_PAWN);
	ret = fc_board_get_piece(&board, &player, &piece, 6, 7);
	fail_unless(ret == 1 && player == FC_SECOND && piece == FC_KING);
	ret = fc_board_get_piece(&board, &player, &piece, 4, 3);
	fail_unless(ret == 1 && player == FC_THIRD && piece == FC_QUEEN);
	ret = fc_board_get_piece(&board, &player, &piece, 3, 4);
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

	fc_board_set_piece(&board, FC_FIRST, FC_KING, 0, 0);
	fc_board_set_piece(&board, FC_SECOND, FC_PAWN, 2, 2);
	fc_board_set_piece(&board, FC_SECOND, FC_KING, 3, 3);
	fc_board_set_piece(&board, FC_THIRD, FC_KING, 7, 7);
	fc_board_set_piece(&board, FC_FOURTH, FC_PAWN, 4, 4);

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

	fc_mlist_free(&moves);
}
END_TEST

extern void fc_get_knight_moves (fc_board_t *board,
				 fc_mlist_t *moves,
				 fc_piece_t piece);

START_TEST (test_forchess_knight_moves)
{
	fc_board_t board;
	bzero(board, sizeof(board));

	fc_board_set_piece(&board, FC_FIRST, FC_KNIGHT, 2, 2);
	fc_board_set_piece(&board, FC_SECOND, FC_KNIGHT, 1, 0);
	fc_board_set_piece(&board, FC_THIRD, FC_KNIGHT, 0, 1);
	fc_board_set_piece(&board, FC_FOURTH, FC_KNIGHT, 7, 7);
	fc_board_set_piece(&board, FC_FOURTH, FC_KNIGHT, 5, 6);

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

	fc_mlist_free(&moves);
}
END_TEST

extern void fc_get_pawn_moves (fc_board_t *board,
			       fc_mlist_t *moves,
			       fc_player_t player);

START_TEST (test_forchess_pawn_moves)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_board_set_piece(&board, FC_FIRST, FC_PAWN, 1, 3);
	fc_board_set_piece(&board, FC_FIRST, FC_PAWN, 3, 3);
	fc_board_set_piece(&board, FC_SECOND, FC_PAWN, 1, 4);
	fc_board_set_piece(&board, FC_SECOND, FC_PAWN, 4, 3);
	fc_board_set_piece(&board, FC_THIRD, FC_PAWN, 4, 4);
	fc_board_set_piece(&board, FC_THIRD, FC_PAWN, 5, 3);
	fc_board_set_piece(&board, FC_FOURTH, FC_PAWN, 0, 3);
	fc_board_set_piece(&board, FC_FOURTH, FC_PAWN, 5, 4);

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

	fc_mlist_free(&moves);
}
END_TEST

extern void fc_get_bishop_moves (fc_board_t *board,
				 fc_mlist_t *moves,
				 fc_player_t player);

START_TEST (test_forchess_bishop_moves)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_board_set_piece(&board, FC_FIRST, FC_BISHOP, 5, 3);
	fc_board_set_piece(&board, FC_SECOND, FC_BISHOP, 2, 6);
	fc_board_set_piece(&board, FC_THIRD, FC_BISHOP, 2, 3);
	fc_board_set_piece(&board, FC_THIRD, FC_PAWN, 2, 0);
	fc_board_set_piece(&board, FC_FOURTH, FC_BISHOP, 4, 1);
	fc_board_set_piece(&board, FC_FOURTH, FC_PAWN, 3, 7);

	fc_mlist_t moves;
	fc_mlist_init(&moves, 0);
	fc_get_bishop_moves(&board, &moves, FC_FIRST);
	fail_unless(fc_mlist_length(&moves) == 9);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("d6-c7"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("d6-b8"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("d6-c5"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("d6-b4"));
	fail_unless(fc_mlist_get(&moves, 4)->move == fc_uint64("d6-e7"));
	fail_unless(fc_mlist_get(&moves, 5)->move == fc_uint64("d6-f8"));
	fail_unless(fc_mlist_get(&moves, 6)->move == fc_uint64("d6-e5"));
	fail_unless(fc_mlist_get(&moves, 7)->move == fc_uint64("d6-f4"));
	fail_unless(fc_mlist_get(&moves, 8)->move == fc_uint64("d6-g3"));

	fc_mlist_clear(&moves);
	fc_get_bishop_moves(&board, &moves, FC_SECOND);
	fail_unless(fc_mlist_length(&moves) == 6);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("g3-f4"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("g3-e5"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("g3-d6"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("g3-f2"));
	fail_unless(fc_mlist_get(&moves, 4)->move == fc_uint64("g3-e1"));
	fail_unless(fc_mlist_get(&moves, 5)->move == fc_uint64("g3-h2"));

	fc_mlist_clear(&moves);
	fc_get_bishop_moves(&board, &moves, FC_THIRD);
	fail_unless(fc_mlist_length(&moves) == 10);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("d3-c4"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("d3-b5"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("d3-c2"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("d3-b1"));
	fail_unless(fc_mlist_get(&moves, 4)->move == fc_uint64("d3-e4"));
	fail_unless(fc_mlist_get(&moves, 5)->move == fc_uint64("d3-f5"));
	fail_unless(fc_mlist_get(&moves, 6)->move == fc_uint64("d3-g6"));
	fail_unless(fc_mlist_get(&moves, 7)->move == fc_uint64("d3-h7"));
	fail_unless(fc_mlist_get(&moves, 8)->move == fc_uint64("d3-e2"));
	fail_unless(fc_mlist_get(&moves, 9)->move == fc_uint64("d3-f1"));

	fc_mlist_clear(&moves);
	fc_get_bishop_moves(&board, &moves, FC_FOURTH);
	fail_unless(fc_mlist_length(&moves) == 7);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("b5-a6"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("b5-a4"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("b5-c6"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("b5-d7"));
	fail_unless(fc_mlist_get(&moves, 4)->move == fc_uint64("b5-e8"));
	fail_unless(fc_mlist_get(&moves, 5)->move == fc_uint64("b5-c4"));
	fail_unless(fc_mlist_get(&moves, 6)->move == fc_uint64("b5-d3"));

	fc_mlist_free(&moves);
}
END_TEST

extern void fc_get_rook_moves (fc_board_t *board,
			       fc_mlist_t *moves,
			       fc_player_t player);

START_TEST (test_forchess_rook_moves)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_board_set_piece(&board, FC_FIRST, FC_ROOK, 1, 0);
	fc_board_set_piece(&board, FC_FIRST, FC_ROOK, 0, 1);
	fc_board_set_piece(&board, FC_SECOND, FC_ROOK, 5, 1);
	fc_board_set_piece(&board, FC_SECOND, FC_ROOK, 4, 1);
	fc_board_set_piece(&board, FC_THIRD, FC_ROOK, 3, 0);
	fc_board_set_piece(&board, FC_THIRD, FC_ROOK, 4, 5);
	fc_board_set_piece(&board, FC_FOURTH, FC_ROOK, 2, 3);
	fc_board_set_piece(&board, FC_FOURTH, FC_ROOK, 5, 6);

	/* TODO fill in all the moves for these */
	fc_mlist_t moves;
	fc_mlist_init(&moves, 0);
	fc_get_rook_moves(&board, &moves, FC_FIRST);
	fail_unless(fc_mlist_length(&moves) == 20);

	fc_mlist_clear(&moves);
	fc_get_rook_moves(&board, &moves, FC_SECOND);
	fail_unless(fc_mlist_length(&moves) == 16);

	fc_mlist_clear(&moves);
	fc_get_rook_moves(&board, &moves, FC_THIRD);
	fail_unless(fc_mlist_length(&moves) == 25);

	fc_mlist_clear(&moves);
	fc_get_rook_moves(&board, &moves, FC_FOURTH);
	fail_unless(fc_mlist_length(&moves) == 26);

	fc_mlist_free(&moves);
}
END_TEST

extern void fc_get_queen_moves (fc_board_t *board,
				fc_mlist_t *moves,
				fc_player_t player);

START_TEST (test_forchess_queen_moves)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_board_set_piece(&board, FC_FIRST, FC_QUEEN, 1, 0);
	fc_board_set_piece(&board, FC_FIRST, FC_QUEEN, 0, 1);
	fc_board_set_piece(&board, FC_SECOND, FC_QUEEN, 5, 2);
	fc_board_set_piece(&board, FC_SECOND, FC_QUEEN, 4, 1);
	fc_board_set_piece(&board, FC_THIRD, FC_QUEEN, 3, 0);
	fc_board_set_piece(&board, FC_THIRD, FC_QUEEN, 4, 5);
	fc_board_set_piece(&board, FC_FOURTH, FC_QUEEN, 2, 3);
	fc_board_set_piece(&board, FC_FOURTH, FC_QUEEN, 5, 6);

	/* TODO fill in all the moves for these */
	fc_mlist_t moves;
	fc_mlist_init(&moves, 0);
	fc_get_queen_moves(&board, &moves, FC_FIRST);
	fail_unless(fc_mlist_length(&moves) == 15 + 13);

	fc_mlist_clear(&moves);
	fc_get_queen_moves(&board, &moves, FC_SECOND);
	fail_unless(fc_mlist_length(&moves) == 15 + 21);

	fc_mlist_clear(&moves);
	fc_get_queen_moves(&board, &moves, FC_THIRD);
	fail_unless(fc_mlist_length(&moves) == 16 + 21);

	fc_mlist_clear(&moves);
	fc_get_queen_moves(&board, &moves, FC_FOURTH);
	fail_unless(fc_mlist_length(&moves) == 21 + 16);

	fc_mlist_free(&moves);
}
END_TEST

START_TEST (test_forchess_get_removes)
{
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_board_set_piece(&board, FC_FIRST, FC_KING, 6, 6);
	fc_board_set_piece(&board, FC_FIRST, FC_QUEEN, 5, 5);
	fc_board_set_piece(&board, FC_FIRST, FC_ROOK, 4, 4);
	fc_board_set_piece(&board, FC_FIRST, FC_BISHOP, 3, 3);
	fc_board_set_piece(&board, FC_FIRST, FC_KNIGHT, 2, 2);
	fc_board_set_piece(&board, FC_FIRST, FC_PAWN, 1, 1);

	/* TODO fill in all the removes for these */
	fc_mlist_t rm;
	fc_mlist_init(&rm, 0);
	fc_board_get_removes(&board, &rm, FC_FIRST);
	fail_unless(fc_mlist_length(&rm) == 6);

	fc_mlist_free(&rm);
}
END_TEST

START_TEST (test_forchess_make_move)
{
	/* (1) check basic moves */
	fc_board_t board;
	bzero(&board, sizeof(board));
	fc_move_t move;
	move.player = FC_FOURTH;
	move.piece = FC_KNIGHT;
	fc_board_set_piece(&board, move.player, move.piece, 2, 2);
	move.move = fc_uint64("c3-e2");
	fc_board_make_move(&board, &move);
	fc_player_t player;
	fc_piece_t piece;
	fail_unless(!fc_board_get_piece(&board, &player, &piece, 2, 2));
	fail_unless(fc_board_get_piece(&board, &player, &piece, 1, 4));
	fail_unless(player == move.player && piece == move.piece);
	/* (2) check captures */
	move.player = FC_FIRST;
	move.piece = FC_ROOK;
	fc_board_set_piece(&board, move.player, move.piece, 4, 4);
	move.move = fc_uint64("e5-e2");
	fc_board_make_move(&board, &move);
	fail_unless(fc_board_get_piece(&board, &player, &piece, 1, 4));
	fail_unless(player == move.player && piece == move.piece);
	fail_unless(board[FC_FOURTH * 6 + FC_KNIGHT] == UINT64_C(0));
	/* (3) check removes */
	move.move = UINT64_C(0x1000);
	fc_board_make_move(&board, &move);
	for (int i = 0; i < 24; i++) {
		fail_unless(board[i] == UINT64_C(0));
	}
	/* (4) check that pieces change sides on capture of king */
	fc_board_setup(&board, "test/boards/test_forchess_make_move.1");
	move.player = FC_FIRST;
	move.piece = FC_KNIGHT;
	move.move = fc_uint64("b6-a8");
	fc_board_make_move(&board, &move);
	fail_unless(board[FC_SECOND * 6 + FC_KING] == UINT64_C(0));
	fail_unless(fc_board_get_piece(&board, &player, &piece, 0, 0));
	fail_unless(player == FC_FIRST);
	fail_unless(fc_board_get_piece(&board, &player, &piece, 0, 7));
	fail_unless(player == FC_FIRST);
	fail_unless(fc_board_get_piece(&board, &player, &piece, 2, 1));
	fail_unless(player == FC_FOURTH);
	/* (5) check that pawns don't change orientation if they change sides */
	fail_unless(fc_board_get_piece(&board, &player, &piece, 7, 3));
	fail_unless(player == FC_FIRST);
	fc_mlist_t moves;
	fc_mlist_init(&moves, 0);
	fc_get_rook_moves(&board, &moves, FC_FIRST);
	fail_unless(fc_mlist_length(&moves) == 7);
	fc_mlist_clear(&moves);
	fc_get_pawn_moves(&board, &moves, FC_FIRST);
	fail_unless(fc_mlist_length(&moves) == 4);
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("d6-e6"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("d8-e7"));
	/* (5a) ... even after changing sides multiple times */
	move.player = FC_FOURTH;
	move.piece = FC_BISHOP;
	move.move = fc_uint64("b3-c2");
	fc_board_make_move(&board, &move);
	fc_mlist_clear(&moves);
	fc_get_pawn_moves(&board, &moves, FC_FOURTH);
	fail_unless(fc_mlist_length(&moves) == 4);
	fail_unless(fc_mlist_get(&moves, 0)->move == fc_uint64("d4-e5"));
	fail_unless(fc_mlist_get(&moves, 1)->move == fc_uint64("d6-e5"));
	fail_unless(fc_mlist_get(&moves, 2)->move == fc_uint64("e6-d7"));
	fail_unless(fc_mlist_get(&moves, 3)->move == fc_uint64("d8-e7"));
	/* TODO for pawns:
	 * 	in fc_board_set_piece() add a condtion to update the new
	 * 	pawn orientation bitboards if a pawn is set on the board
	 *
	 * 	check for orientation in fc_get_pawn_moves(); switch on
	 * 	orientation before calling pawn_move_if_valid(), but you should
	 * 	be able to keep everything else the same
	 *
	 * 	when exercising fc_board_make_move() you'll need to update pawn
	 * 	moves on both the pawn bitboard and orientation bitboard; same
	 * 	goes for conversion
	 */
	/* (6) check that moving a pawn to its backboard returns 0, and write
	 * fc_board_make_pawn_move() */
	fail_unless(fc_board_make_move(&board, fc_mlist_get(&moves, 2)));
	move.player = FC_FOURTH;
	move.piece = FC_PAWN;
	move.move = fc_uint64("d7-c8");
	fail_unless(!fc_board_make_move(&board, &move));
	fail_unless(fc_board_make_pawn_move(&board, &move, FC_QUEEN));
	fc_mlist_clear(&moves);
	fc_get_queen_moves(&board, &moves, FC_FOURTH);
	fail_unless(fc_mlist_length(&moves) != 0);

	fc_mlist_free(&moves);
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
	tcase_add_test(tc_board, test_forchess_bishop_moves);
	tcase_add_test(tc_board, test_forchess_rook_moves);
	tcase_add_test(tc_board, test_forchess_queen_moves);
	tcase_add_test(tc_board, test_forchess_get_removes);
	tcase_add_test(tc_board, test_forchess_make_move);
	suite_add_tcase(s, tc_board);
	return s;
}
