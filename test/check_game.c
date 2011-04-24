#include <check.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "forchess/board.h"
#include "forchess/game.h"

START_TEST (test_forchess_game_save_load)
{
	fc_game_t game;
	fc_game_init(&game);
	fail_unless(fc_game_load(&game,
				"test/boards/test_forchess_game_save_load.1"));
	fail_unless(game.player == FC_FIRST);
	game.player = FC_THIRD;
	fail_unless(fc_game_save(&game, "/tmp/test.fc"));
	fc_game_free(&game);
	fc_game_init(&game);
	fail_unless(fc_game_load(&game, "/tmp/test.fc"));
	fail_unless(game.player == FC_THIRD);

	fc_player_t player;
	fc_piece_t piece;
	fail_unless(fc_board_get_piece(game.board, &player, &piece, 2, 0));
	fail_unless(player == FC_FIRST && piece == FC_KNIGHT);

	fc_game_free(&game);
}
END_TEST

START_TEST (test_forchess_game_next_player)
{
	fc_game_t game;
	fc_game_init(&game);
	game.player = FC_FIRST;
	fc_board_set_piece(game.board, FC_FIRST, FC_KING, 0, 1);
	fc_board_set_piece(game.board, FC_THIRD, FC_KING, 0, 2);
	fc_board_set_piece(game.board, FC_FOURTH, FC_KING, 0, 3);
	fail_unless(fc_game_next_player(&game) == FC_THIRD);
	fail_unless(fc_game_next_player(&game) == FC_FOURTH);
	fail_unless(fc_game_next_player(&game) == FC_FIRST);
	fc_game_free(&game);
}
END_TEST

START_TEST (test_forchess_game_num_players)
{
	fc_game_t game;
	fc_game_init(&game);
	game.player = FC_FIRST;
	fc_board_set_piece(game.board, FC_FIRST, FC_KING, 0, 1);
	fc_board_set_piece(game.board, FC_THIRD, FC_KING, 0, 2);
	fc_board_set_piece(game.board, FC_FOURTH, FC_KING, 0, 3);
	fail_unless(fc_game_number_of_players(&game) == 3);
	fc_game_free(&game);
}
END_TEST

START_TEST (test_forchess_game_move_conversions)
{
	fc_game_t game;
	fc_game_init(&game);
	game.player = FC_FIRST;
	fc_board_set_piece(game.board, FC_FIRST, FC_KING, 0, 0);
	fc_mlist_t list;
	fc_mlist_init(&list, 0);
	fc_board_get_moves(game.board, &list, FC_FIRST);
	int w, x, y, z;
	fc_move_t *move = fc_mlist_get(&list, 0);
	fail_unless(fc_game_convert_move_to_coords(&game, &w, &x, &y, &z,
				move));
	fc_move_t other;
	fail_unless(fc_game_convert_coords_to_move(&game, &other, w, x, y, z));
	fail_unless(move->player == other.player &&
		    move->piece == other.piece &&
		    move->move == other.move);
	fc_mlist_free(&list);
	fc_game_free(&game);
}
END_TEST

START_TEST (test_forchess_game_over)
{
	fc_game_t game;
	fc_game_init(&game);
	game.player = FC_FIRST;
	fc_board_set_piece(game.board, FC_FIRST, FC_KING, 0, 1);
	fc_board_set_piece(game.board, FC_THIRD, FC_KING, 0, 2);
	fail_unless(fc_game_is_over(&game));
	fc_game_free(&game);
}
END_TEST

START_TEST (test_forchess_game_is_move_valid)
{
	fc_game_t game;
	fc_game_init(&game);
	fc_game_load(&game, "test/boards/test_forchess_game_is_move_valid.1");
	fc_move_t move;
	move.player = FC_FOURTH;
	move.piece = FC_ROOK;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = fc_uint64("g2-h2");
	game.player = FC_FOURTH;
	/* test that legal moves are valid */
	fail_unless(fc_game_is_move_legal(&game, &move));

	move.move = fc_uint64("g2-h3");
	/* test that illegal moves are invalid */
	fail_unless(!fc_game_is_move_legal(&game, &move));

	move.player = FC_THIRD;
	move.piece = FC_PAWN;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.move = FC_BITBOARD((*game.board), FC_THIRD, FC_PAWN);
	game.player = FC_THIRD;
	/* test that we can't remove a piece if we have valid moves available */
	fail_unless(!fc_game_is_move_legal(&game, &move));

	move.player = FC_FIRST;
	move.piece = FC_KING;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.move = FC_BITBOARD((*game.board), FC_FIRST, FC_KING);
	game.player = FC_FIRST;
	/* test that we can't remove the king until we have exhausted all other
	 * options */
	fail_unless(!fc_game_is_move_legal(&game, &move));

	move.piece = FC_PAWN;
	move.move = FC_BITBOARD((*game.board), FC_FIRST, FC_PAWN);
	/* test that legal removes are valid */
	fail_unless(fc_game_is_move_legal(&game, &move));

	fc_game_free(&game);
	fc_game_init(&game);
	fc_game_load(&game, "test/boards/test_forchess_game_is_move_valid.2");
	move.move = FC_BITBOARD((*game.board), FC_FIRST, FC_PAWN);
	/* test that we can't remove a piece if it will put us in check */
	fail_unless(!fc_game_is_move_legal(&game, &move));
	move.piece = FC_KNIGHT;
	move.move = FC_BITBOARD((*game.board), FC_FIRST, FC_KNIGHT);
	fail_unless(fc_game_is_move_legal(&game, &move));

	fc_game_free(&game);
	fc_game_init(&game);
	fc_game_load(&game, "test/boards/test_forchess_game_is_move_valid.3");
	move.piece = FC_PAWN;
	move.move = FC_BITBOARD((*game.board), FC_FIRST, FC_PAWN);
	/* test that we can remove a piece if it will put us in check if it is
	 * our only option */
	fail_unless(fc_game_is_move_legal(&game, &move));

	fc_board_remove_piece(game.board, 0, 1);
	move.piece = FC_KING;
	move.move = FC_BITBOARD((*game.board), FC_FIRST, FC_KING);
	/* test that we can remove the king if he is our only piece and we have
	 * no valid moves */
	fail_unless(fc_game_is_move_legal(&game, &move));
	fc_game_free(&game);
}
END_TEST

START_TEST (test_forchess_game_opp_check_status)
{
	fc_game_t game;
	fc_game_init(&game);
	fc_game_load(&game,
			"test/boards/test_forchess_game_opp_check_status.1");
	fc_move_t move;
	move.player = FC_FOURTH;
	move.piece = FC_ROOK;
	move.opp_player = FC_FIRST;
	move.opp_piece = FC_PAWN;
	move.promote = FC_NONE;
	move.move = fc_uint64("g1-b1");
	fail_unless(fc_game_opponent_kings_check_status(&game, FC_FOURTH,
				&move) == FC_CHECK);
	move.piece = FC_QUEEN;
	move.move = fc_uint64("e4-b1");
	fail_unless(fc_game_opponent_kings_check_status(&game, FC_FOURTH,
				&move) == FC_CHECKMATE);
}
END_TEST

Suite *game_suite (void)
{
	Suite *s = suite_create("Game");
	TCase *tc_game = tcase_create("Core");
	tcase_add_test(tc_game, test_forchess_game_save_load);
	tcase_add_test(tc_game, test_forchess_game_next_player);
	tcase_add_test(tc_game, test_forchess_game_num_players);
	tcase_add_test(tc_game, test_forchess_game_move_conversions);
	tcase_add_test(tc_game, test_forchess_game_over);
	tcase_add_test(tc_game, test_forchess_game_is_move_valid);
	tcase_add_test(tc_game, test_forchess_game_opp_check_status);
	suite_add_tcase(s, tc_game);
	return s;
}
