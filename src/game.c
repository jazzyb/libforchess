#include <assert.h>
#include <stdlib.h>
#include <strings.h>

#include "forchess/ai.h"
#include "forchess/board.h"
#include "forchess/game.h"
#include "forchess/moves.h"

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

static void remove2position (char *str, uint64_t pos)
{
	int i = 0;
	while (pos) {
		pos >>= 1;
		i++;
	}
	char x = ((i - 1) % 8) + 'a';
	char y = ((i - 1) / 8) + '1';
	sprintf(str, "%c%c", x, y);
}

static inline char piece2char (fc_piece_t piece)
{
	switch (piece) {
	case FC_PAWN:
		return 'P';
	case FC_BISHOP:
		return 'B';
	case FC_KNIGHT:
		return 'N';
	case FC_ROOK:
		return 'R';
	case FC_QUEEN:
		return 'Q';
	case FC_KING:
		return 'K';
	}
	/* should never get here */
	assert(0);
	return '\0';
}

int fc_game_save (fc_game_t *game, const char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (!fp) {
		return 0;
	}

	fc_mlist_t list;
	fc_mlist_init(&list, 0);
	fc_board_get_removes(game->board, &list, game->player);
	for (int i = 0; i < fc_mlist_length(&list); i++) {
		fc_move_t *move = fc_mlist_get(&list, i);
		char str[3];
		remove2position(str, move->move);
		fprintf(fp, "%d %c %s\n", game->player + 1,
				piece2char(move->piece), str);
	}

	for (fc_player_t player = FC_NEXT_PLAYER(game->player);
	     player != game->player;
	     player = FC_NEXT_PLAYER(player)) {
		fc_mlist_clear(&list);
		fc_board_get_removes(game->board, &list, player);
		for (int i = 0; i < fc_mlist_length(&list); i++) {
			fc_move_t *move = fc_mlist_get(&list, i);
			char str[3];
			remove2position(str, move->move);
			fprintf(fp, "%d %c %s\n", player + 1,
					piece2char(move->piece), str);
		}
	}

	fc_mlist_free(&list);
	fclose(fp);
	return 1;
}

int fc_game_load (fc_game_t *game, const char *filename)
{
	return fc_board_setup(game->board, filename, &(game->player));
}

fc_player_t fc_game_current_player (fc_game_t *game)
{
	return game->player;
}

fc_player_t fc_game_next_player (fc_game_t *game)
{
	fc_player_t next;
	for (next = FC_NEXT_PLAYER(game->player);
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

/*
 * Returns FC_CHECK if the given move will put one of the opponent kings in
 * check; FC_CHECKMATE if checkmate.  Returns 0 if neither is true.
 */
int fc_game_opponent_kings_check_status (fc_game_t *game, fc_player_t player,
		fc_move_t *move)
{
	fc_board_t copy;
	fc_board_copy(&copy, game->board);
	fc_board_make_move(&copy, move);
	int check_status_before = fc_is_king_in_check(game->board,
			FC_NEXT_PLAYER(player));
	int check_status_after = fc_is_king_in_check(&copy,
			FC_NEXT_PLAYER(player));
	if (!check_status_before && check_status_after == FC_CHECK) {
		return FC_CHECK;
	} else if (check_status_before != FC_CHECKMATE &&
			check_status_after == FC_CHECKMATE) {
		return FC_CHECKMATE;
	}

	check_status_before = fc_is_king_in_check(game->board,
			FC_PARTNER(FC_NEXT_PLAYER(player)));
	check_status_after = fc_is_king_in_check(&copy,
			FC_PARTNER(FC_NEXT_PLAYER(player)));
	if (!check_status_before && check_status_after == FC_CHECK) {
		return FC_CHECK;
	} else if (check_status_before != FC_CHECKMATE &&
			check_status_after == FC_CHECKMATE) {
		return FC_CHECKMATE;
	}

	return 0;
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
	uint64_t m = FC_BITBOARD((*(game->board)), move->player, move->piece) &
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
	if (!fc_board_get_piece(game->board, &(move->player), &(move->piece),
				y1, x1)) {
		return 0;
	}
	move->move = UINT64_C(1) << ((y1 * 8) + x1);
	if (x2 != -1 && y2 != -1) {
		move->move |= UINT64_C(1) << ((y2 * 8) + x2);
	}
	return 1;
}
