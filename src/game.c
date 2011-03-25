#include "forchess/game.h"

/*
 * A wrapper around the forchess API.
 */

int fc_game_init (fc_game_t *game)
{
	game->board = calloc(1, sizeof(fc_board_t));
	if (!game->board) {
		return 0;
	}
	bzero(game->board, sizeof(fc_board_t));
	game->player = FC_NONE;
	return 1;
}

void fc_game_free (fc_game_t *game)
{
	free(game->board);
}

fc_player_t fc_game_current_player (fc_game_t *game)
{
	return game->player;
}

fc_player_t fc_game_next_player (fc_game_t *game)
{
	fc_player_t next;
	for (next = FC_NEXT_PLAYER(player);
	     !FC_BITBOARD((*(game->board)), next, FC_KING);
	     next = FC_NEXT_PLAYER(next));
	game->player = next;
	return next;
}

int fc_game_number_of_players (fc_game_t *game)
{
	return !!FC_BITBOARD((*(game->board)), FC_FIRST, FC_KING) +
	       !!FC_BITBOARD((*(game->board)), FC_SECOND, FC_KING) +
	       !!FC_BITBOARD((*(game->board)), FC_THIRD, FC_KING) +
	       !!FC_BITBOARD((*(game->board)), FC_FOURTH, FC_KING);
}

int fc_game_king_check_status (fc_game_t *game, fc_player_t player)
{
	return fc_is_king_in_check(game->board, player);
}

int fc_game_is_move_valid (fc_game_t *game, fc_move_t *move)
{
	if (move->player != game->player) {
		return 0;
	}
	fc_mlist_t list;
	fc_mlist_init(&list, 0);
	fc_board_get_moves(game->board, &list, move->player);
	for (int i = 0; i < fc_mlist_length(&list); i++) {
		fc_move_t *other = fc_mlist_get(&list, i);
		if (move->piece == other->piece && move->move == other->move) {
			fc_mlist_free(&list);
			return fc_ai_is_move_valid(game->board, move);
		}
	}
	fc_mlist_free(&list);
	return 0;
}

int fc_game_is_over (fc_game_t *game)
{
	return ((!FC_BITBOARD((*(game->board)), FC_FIRST, FC_KING) &&
			!FC_BITBOARD((*(game->board)), FC_THIRD, FC_KING)) ||
		(!FC_BITBOARD((*(game->board)), FC_SECOND, FC_KING) &&
		 	!FC_BITBOARD((*(game->board)), FC_FOURTH, FC_KING)));
}

int fc_game_make_move (fc_game_t *game, fc_move_t *move)
{
	return fc_board_make_move(game->board, move);
}

void fc_game_set_promote_pawn (fc_move_t *move, fc_piece_t promote)
{
	move->promote = promote;
}

int fc_game_convert_move_to_coords (fc_game_t *game, int *x1, int *y1,
		int *x2, int *y2, fc_move_t *move)
{
	uint64_t m = FC_BITBOARD((*board), move->player, move->piece) &
			move->move;
	if (!m) {
		return 0;
	}
	int i = 0;
	uint64_t bit = m;
	while (bit) {
		bit >>= 1;
		i++;
	}
	*y1 = ((i - 1) / 8);
	*x1 = ((i - 1) % 8);

	m ^= move->move;
	i = 0;
	bit = m;
	while (bit) {
		bit >>= 1;
		i++;
	}
	if (i) {
		*y2 = ((i - 1) / 8);
		*x2 = ((i - 1) % 8);
	} else {
		*x2 = *y2 = -1;
	}
	return 1;
}

int fc_game_convert_coords_to_move (fc_game_t *game, fc_move_t *move,
		int x1, int y1, int x2, int y2)
{
	move->promote = FC_NONE;
	/* FIXME Notice how I have to flip the x and y.  That's confusing. */
	if (!fc_board_get_piece(board, &(move->player), &(move->piece),
				y1, x1)) {
		return 0;
	}
	move->move = UINT64_C(1) << ((y1 * 8) + x1);
	if (x2 != -1 && y2 != -1) {
		move->move |= UINT64_C(1) << ((y2 * 8) + x2);
	}
	return 1;
}
