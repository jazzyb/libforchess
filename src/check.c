/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>

#include "forchess/board.h"

static int is_threatened_by_king (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t enemy_kings;

	enemy_kings = FC_BITBOARD(board, FC_NEXT_PLAYER(player), FC_KING) |
		FC_BITBOARD(board, FC_PARTNER(FC_NEXT_PLAYER(player)),
				FC_KING);
	if (!(king & FC_LEFT_COL)) {
		if (((king << 7) & enemy_kings) ||
		    ((king >> 9) & enemy_kings) ||
		    ((king >> 1) & enemy_kings)) {
			return 1;
		}
	}
	if (((king >> 8) & enemy_kings) || ((king << 8) & enemy_kings)) {
		return 1;
	}
	if (!(king & FC_RIGHT_COL)) {
		if (((king << 9) & enemy_kings) ||
		    ((king >> 7) & enemy_kings) ||
		    ((king << 1) & enemy_kings)) {
			return 1;
		}
	}
	return 0;
}

/* TODO double check the below function -- it needs to be PERFECT */
static int is_threatened_by_pawn (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t pawns1, pawns2, pawns3, pawns4;

	switch (player) {
	case FC_FIRST:
		/* player 2 pawns that are originally player 2's pawns */
		pawns2 = FC_BITBOARD(board, FC_SECOND, FC_PAWN) &
			 FC_PAWN_BB(board, FC_SECOND);
		/* player 2 pawns that (after TWO king captures) are under
		 * player 4's control */
		pawns2 |= FC_BITBOARD(board, FC_FOURTH, FC_PAWN) &
			  FC_PAWN_BB(board, FC_SECOND);
		/* partner pawns which are under enemy control */
		pawns3 = FC_BITBOARD(board, FC_SECOND, FC_PAWN) &
			 FC_PAWN_BB(board, FC_THIRD);
		pawns3 |= FC_BITBOARD(board, FC_FOURTH, FC_PAWN) &
			  FC_PAWN_BB(board, FC_THIRD);
		/* player 4 pawns that are originally player 4's pawns */
		pawns4 = FC_BITBOARD(board, FC_FOURTH, FC_PAWN) &
			 FC_PAWN_BB(board, FC_FOURTH);
		/* player 4 pawns that (after TWO king captures) are under
		 * player 2's control */
		pawns4 |= FC_BITBOARD(board, FC_SECOND, FC_PAWN) &
			  FC_PAWN_BB(board, FC_FOURTH);
		/* check left */
		if (!(king & FC_LEFT_COL) && ((king >> 1) & pawns2)) {
			return 1;
		}
		/* check right */
		if (!(king & FC_RIGHT_COL) &&
			(((king << 1) & pawns4) || ((king << 1) & pawns3))) {
			return 1;
		}
		/* check up */
		if (((king << 8) & pawns2) || ((king << 8) & pawns3)) {
			return 1;
		}
		/* check down */
		if ((king >> 8) & pawns4) {
			return 1;
		}
		break;
	case FC_SECOND:
		/* player 1 pawns that are originally player 1's pawns */
		pawns1 = FC_BITBOARD(board, FC_FIRST, FC_PAWN) &
			 FC_PAWN_BB(board, FC_FIRST);
		/* player 1 pawns that (after TWO king captures) are under
		 * player 3's control */
		pawns1 |= FC_BITBOARD(board, FC_THIRD, FC_PAWN) &
			  FC_PAWN_BB(board, FC_FIRST);
		/* partner pawns which are under enemy control */
		pawns4 = FC_BITBOARD(board, FC_FIRST, FC_PAWN) &
			 FC_PAWN_BB(board, FC_FOURTH);
		pawns4 |= FC_BITBOARD(board, FC_THIRD, FC_PAWN) &
			  FC_PAWN_BB(board, FC_FOURTH);
		/* player 3 pawns that are originally player 3's pawns */
		pawns3 = FC_BITBOARD(board, FC_THIRD, FC_PAWN) &
			 FC_PAWN_BB(board, FC_THIRD);
		/* player 3 pawns that (after TWO king captures) are under
		 * player 1's control */
		pawns3 |= FC_BITBOARD(board, FC_FIRST, FC_PAWN) &
			  FC_PAWN_BB(board, FC_THIRD);
		/* check left */
		if (!(king & FC_LEFT_COL) && ((king >> 1) & pawns1)) {
			return 1;
		}
		/* check right */
		if (!(king & FC_RIGHT_COL) &&
			(((king << 1) & pawns3) || ((king << 1) & pawns4))) {
			return 1;
		}
		/* check up */
		if ((king << 8) & pawns3) {
			return 1;
		}
		/* check down */
		if (((king >> 8) & pawns1) || ((king >> 8) & pawns4)) {
			return 1;
		}
		break;
	case FC_THIRD:
		/* player 2 pawns that are originally player 2's pawns */
		pawns2 = FC_BITBOARD(board, FC_SECOND, FC_PAWN) &
			 FC_PAWN_BB(board, FC_SECOND);
		/* player 2 pawns that (after TWO king captures) are under
		 * player 4's control */
		pawns2 |= FC_BITBOARD(board, FC_FOURTH, FC_PAWN) &
			  FC_PAWN_BB(board, FC_SECOND);
		/* partner pawns which are under enemy control */
		pawns1 = FC_BITBOARD(board, FC_SECOND, FC_PAWN) &
			 FC_PAWN_BB(board, FC_FIRST);
		pawns1 |= FC_BITBOARD(board, FC_FOURTH, FC_PAWN) &
			  FC_PAWN_BB(board, FC_FIRST);
		/* player 4 pawns that are originally player 4's pawns */
		pawns4 = FC_BITBOARD(board, FC_FOURTH, FC_PAWN) &
			 FC_PAWN_BB(board, FC_FOURTH);
		/* player 4 pawns that (after TWO king captures) are under
		 * player 2's control */
		pawns4 |= FC_BITBOARD(board, FC_SECOND, FC_PAWN) &
			  FC_PAWN_BB(board, FC_FOURTH);
		/* check left */
		if (!(king & FC_LEFT_COL) &&
			(((king >> 1) & pawns2) || ((king >> 1) & pawns1))) {
			return 1;
		}
		/* check right */
		if (!(king & FC_RIGHT_COL) && ((king << 1) & pawns4)) {
			return 1;
		}
		/* check up */
		if ((king << 8) & pawns2) {
			return 1;
		}
		/* check down */
		if (((king >> 8) & pawns4) || ((king >> 8) & pawns1)) {
			return 1;
		}
		break;
	case FC_FOURTH:
		/* player 1 pawns that are originally player 1's pawns */
		pawns1 = FC_BITBOARD(board, FC_FIRST, FC_PAWN) &
			 FC_PAWN_BB(board, FC_FIRST);
		/* player 1 pawns that (after TWO king captures) are under
		 * player 3's control */
		pawns1 |= FC_BITBOARD(board, FC_THIRD, FC_PAWN) &
			  FC_PAWN_BB(board, FC_FIRST);
		/* partner pawns which are under enemy control */
		pawns2 = FC_BITBOARD(board, FC_FIRST, FC_PAWN) &
			 FC_PAWN_BB(board, FC_SECOND);
		pawns2 |= FC_BITBOARD(board, FC_THIRD, FC_PAWN) &
			  FC_PAWN_BB(board, FC_SECOND);
		/* player 3 pawns that are originally player 3's pawns */
		pawns3 = FC_BITBOARD(board, FC_THIRD, FC_PAWN) &
			 FC_PAWN_BB(board, FC_THIRD);
		/* player 3 pawns that (after TWO king captures) are under
		 * player 1's control */
		pawns3 |= FC_BITBOARD(board, FC_FIRST, FC_PAWN) &
			  FC_PAWN_BB(board, FC_THIRD);
		/* check left */
		if (!(king & FC_LEFT_COL) &&
			(((king >> 1) & pawns1) || ((king >> 1) & pawns2))) {
			return 1;
		}
		/* check right */
		if (!(king & FC_RIGHT_COL) && ((king << 1) & pawns3)) {
			return 1;
		}
		/* check up */
		if (((king << 8) & pawns3) || ((king << 8) & pawns2)) {
			return 1;
		}
		/* check down */
		if ((king >> 8) & pawns1) {
			return 1;
		}
		break;
	}
	return 0;
}

static int king_in_check_upward (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	uint64_t i;

	for (i = king << 8; i; i <<= 8) {
		if (i & threats) {
			return 1;
		}
		if (!fc_is_empty(board, i)) {
			break;
		}
	}
	return 0;
}

static int king_in_check_downward (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	uint64_t i;

	for (i = king >> 8; i; i >>= 8) {
		if (i & threats) {
			return 1;
		}
		if (!fc_is_empty(board, i)) {
			break;
		}
	}
	return 0;
}

static int king_in_check_leftward (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	uint64_t i;

	if (king & FC_LEFT_COL) {
		return 0;
	}

	for (i = king >> 1; i; i >>= 1) {
		if (i & threats) {
			return 1;
		}
		if (!fc_is_empty(board, i)) {
			break;
		}

		if (i & FC_LEFT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_rightward (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	uint64_t i;

	if (king & FC_RIGHT_COL) {
		return 0;
	}

	for (i = king << 1; i; i <<= 1) {
		if (i & threats) {
			return 1;
		}
		if (!fc_is_empty(board, i)) {
			break;
		}

		if (i & FC_RIGHT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_laterally (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t threats;

	threats = FC_BITBOARD(board, FC_NEXT_PLAYER(player), FC_QUEEN);
	threats |= FC_BITBOARD(board, FC_NEXT_PLAYER(player), FC_ROOK);
	threats |= FC_BITBOARD(board, FC_PARTNER(FC_NEXT_PLAYER(player)),
			FC_QUEEN);
	threats |= FC_BITBOARD(board, FC_PARTNER(FC_NEXT_PLAYER(player)),
			FC_ROOK);

	return (king_in_check_upward(board, player, king, threats) ||
		king_in_check_downward(board, player, king, threats) ||
		king_in_check_leftward(board, player, king, threats) ||
		king_in_check_rightward(board, player, king, threats));
}

static int king_in_check_northwest (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	uint64_t i;

	if (king & FC_LEFT_COL) {
		return 0;
	}

	for (i = king << 7; i; i <<= 7) {
		if (i & threats) {
			return 1;
		}
		if (!fc_is_empty(board, i)) {
			break;
		}
		if (i & FC_LEFT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_southwest (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	uint64_t i;

	if (king & FC_LEFT_COL) {
		return 0;
	}

	for (i = king >> 9; i; i >>= 9) {
		if (i & threats) {
			return 1;
		}
		if (!fc_is_empty(board, i)) {
			break;
		}
		if (i & FC_LEFT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_northeast (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	uint64_t i;

	if (king & FC_RIGHT_COL) {
		return 0;
	}

	for (i = king << 9; i; i <<= 9) {
		if (i & threats) {
			return 1;
		}
		if (!fc_is_empty(board, i)) {
			break;
		}
		if (i & FC_RIGHT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_southeast (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	uint64_t i;

	if (king & FC_RIGHT_COL) {
		return 0;
	}

	for (i = king >> 7; i; i >>= 7) {
		if (i & threats) {
			return 1;
		}
		if (!fc_is_empty(board, i)) {
			break;
		}
		if (i & FC_RIGHT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_diagonally (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t threats;

	threats = FC_BITBOARD(board, FC_NEXT_PLAYER(player), FC_QUEEN);
	threats |= FC_BITBOARD(board, FC_NEXT_PLAYER(player), FC_BISHOP);
	threats |= FC_BITBOARD(board, FC_PARTNER(FC_NEXT_PLAYER(player)),
			FC_QUEEN);
	threats |= FC_BITBOARD(board, FC_PARTNER(FC_NEXT_PLAYER(player)),
			FC_BISHOP);

	return (king_in_check_northwest(board, player, king, threats) ||
		king_in_check_southwest(board, player, king, threats) ||
		king_in_check_northeast(board, player, king, threats) ||
		king_in_check_southeast(board, player, king, threats));
}

static int king_in_check_by_knight (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t knights;

	knights = FC_BITBOARD(board, FC_NEXT_PLAYER(player), FC_KNIGHT) |
		  FC_BITBOARD(board, FC_PARTNER(FC_NEXT_PLAYER(player)),
					  FC_KNIGHT);
	if (!(king & (FC_LEFT_COL | FC_2LEFT_COL))) {
		if (((king << 6) & knights) || ((king >> 10) & knights)) {
			return 1;
		}
	}
	if (!(king & FC_LEFT_COL)) {
		if (((king << 15) & knights) || ((king >> 17) & knights)) {
			return 1;
		}
	}
	if (!(king & (FC_RIGHT_COL | FC_2RIGHT_COL))) {
		if (((king << 10) & knights) || ((king >> 6) & knights)) {
			return 1;
		}
	}
	if (!(king & FC_RIGHT_COL)) {
		if (((king << 17) & knights) || ((king >> 15) & knights)) {
			return 1;
		}
	}
	return 0;
}

/*
 * Returns FC_CHECK (1) if king is in check; 0 otherwise.
 */
static int is_check (fc_board_t *board, fc_player_t player)
{
	uint64_t king;

	king = FC_BITBOARD(board, player, FC_KING);
	if (!king) {
		return 0;
	}

	return (king_in_check_laterally(board, player, king) ||
		king_in_check_diagonally(board, player, king) ||
		is_threatened_by_pawn(board, player, king) ||
		is_threatened_by_king(board, player, king) ||
		king_in_check_by_knight(board, player, king));
}

/*
 * Returns FC_CHECK if player's king is in check, FC_CHECKMATE if checkmate,
 * and 0 otherwise.
 */
int fc_board_check_status (fc_board_t *board, fc_player_t player)
{
	int i;
	fc_mlist_t moves;
	fc_board_t copy;
	fc_move_t *move;

	assert(board);

	if (!is_check(board, player)) {
		return 0;
	}

	fc_mlist_init(&moves);
	fc_board_get_moves(board, &moves, player);
	for (i = 0; i < fc_mlist_length(&moves); i++) {
		fc_board_copy(&copy, board);
		move = fc_mlist_get(&moves, i);
		fc_board_make_move(&copy, move);
		if (!is_check(&copy, player)) {
			/* We must never move such that our opponent is put in
			 * check because of us. Even if it would get us out of
			 * checkmate.  However, I am interpreting this rule to
			 * mean that if our partner is already in check, then
			 * all bets are off.*/
			if (is_check(&copy, FC_PARTNER(player)) &&
			    !is_check(board, FC_PARTNER(player))) {
				continue;
			}
			fc_mlist_free(&moves);
			return FC_CHECK;
		}
	}

	fc_mlist_free(&moves);
	return FC_CHECKMATE;
}

