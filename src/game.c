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
	fc_board_init(game->board);
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
	char x, y;

	while (pos) {
		pos >>= 1;
		i++;
	}
	x = ((i - 1) % 8) + 'a';
	y = ((i - 1) / 8) + '1';
	sprintf(str, "%c%c", x, y);
}

static char piece2char (fc_piece_t piece)
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
	int i;
	char str[3];
	FILE *fp;
	fc_move_t *move;
	fc_mlist_t list;
	fc_player_t player;

	fp = fopen(filename, "w");
	if (!fp) {
		return 0;
	}

	fc_mlist_init(&list, 0);
	fc_board_get_removes(game->board, &list, game->player);
	for (i = 0; i < fc_mlist_length(&list); i++) {
		move = fc_mlist_get(&list, i);
		remove2position(str, move->move);
		fprintf(fp, "%d %c %s\n", game->player + 1,
				piece2char(move->piece), str);
	}

	for (player = FC_NEXT_PLAYER(game->player);
	     player != game->player;
	     player = FC_NEXT_PLAYER(player)) {
		fc_mlist_clear(&list);
		fc_board_get_removes(game->board, &list, player);
		for (i = 0; i < fc_mlist_length(&list); i++) {
			move = fc_mlist_get(&list, i);
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
	bzero(game->board, sizeof(fc_board_t));
	return fc_board_setup(game->board, filename, &(game->player));
}

fc_board_t *fc_game_get_board (fc_game_t *game)
{
	return game->board;
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
	return fc_board_check_status(game->board, player);
}

/*
 * Returns FC_CHECK if the given move will put one of the opponent kings in
 * check; FC_CHECKMATE if checkmate.  Returns 0 if neither is true.
 */
int fc_game_opponent_kings_check_status (fc_game_t *game, fc_player_t player,
		fc_move_t *move)
{
	fc_board_t copy;
	int check_status_before, check_status_after;

	fc_board_copy(&copy, game->board);
	fc_board_make_move(&copy, move);
	check_status_before = fc_board_check_status(game->board,
			FC_NEXT_PLAYER(player));
	check_status_after = fc_board_check_status(&copy,
			FC_NEXT_PLAYER(player));
	if (!check_status_before && check_status_after == FC_CHECK) {
		return FC_CHECK;
	} else if (check_status_before != FC_CHECKMATE &&
			check_status_after == FC_CHECKMATE) {
		return FC_CHECKMATE;
	}

	check_status_before = fc_board_check_status(game->board,
			FC_PARTNER(FC_NEXT_PLAYER(player)));
	check_status_after = fc_board_check_status(&copy,
			FC_PARTNER(FC_NEXT_PLAYER(player)));
	if (!check_status_before && check_status_after == FC_CHECK) {
		return FC_CHECK;
	} else if (check_status_before != FC_CHECKMATE &&
			check_status_after == FC_CHECKMATE) {
		return FC_CHECKMATE;
	}

	return 0;
}

int fc_game_is_move_legal (fc_game_t *game, fc_move_t *move)
{
	int i;
	int valid_move_exists, found_match, valid_remove_exists;
	fc_mlist_t list;
	fc_move_t *other;

	if (move->player != game->player) {
		return 0;
	}

	valid_move_exists = 0;
	fc_mlist_init(&list, 0);
	fc_board_get_moves(game->board, &list, move->player);
	for (i = 0; i < fc_mlist_length(&list); i++) {
		other = fc_mlist_get(&list, i);
		if (move->piece == other->piece && move->move == other->move) {
			fc_mlist_free(&list);
			return fc_ai_is_move_valid(game->board, move);
		}
		valid_move_exists += fc_ai_is_move_valid(game->board, other);
	}
	if (valid_move_exists) {
		fc_mlist_free(&list);
		return 0;
	}

	fc_mlist_clear(&list);
	fc_board_get_removes(game->board, &list, move->player);

	/* Check to see if it is okay to remove the king. */
	if (fc_mlist_length(&list) == 1 && move->piece == FC_KING &&
			move->move == fc_mlist_get(&list, 0)->move) {
		fc_mlist_free(&list);
		return 1;
	}

	found_match = 0;
	valid_remove_exists = 0;
	for (i = 0; i < fc_mlist_length(&list); i++) {
		other = fc_mlist_get(&list, i);
		if (other->piece == FC_KING) {
			continue;
		}
		if (move->piece == other->piece && move->move == other->move) {
			if (fc_ai_is_move_valid(game->board, move)) {
				fc_mlist_free(&list);
				return 1;
			}
			found_match = 1;
		}
		valid_remove_exists += fc_ai_is_move_valid(game->board, other);
	}

	if (found_match && !valid_remove_exists) {
		fc_mlist_free(&list);
		return 1;
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
	int i;
	uint64_t m, bit;

	m = FC_BITBOARD((*(game->board)), move->player, move->piece) &
		move->move;
	if (!m) {
		return 0;
	}
	i = 0;
	bit = m;
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

/* Redefine UINT64_C to be something that plays nicer with the C89 standard */
#undef UINT64_C
#define UINT64_C(x) ((uint64_t)x)

	move->move = UINT64_C(1) << ((y1 * 8) + x1);
	if (x2 != -1 && y2 != -1) {
		move->move |= UINT64_C(1) << ((y2 * 8) + x2);
	}
	return 1;
}

