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
