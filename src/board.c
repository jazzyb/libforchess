#include <stdio.h>
#include <stdlib.h>

#include "forchess/board.h"

/*
 * NOTE:  This macro can also be used as an l-value.
 */
#define FC_BITBOARD(board, player, piece) (board[player * 6 + piece])

int fc_set_piece (fc_board_t *board,
		  fc_player_t player,
		  fc_piece_t piece,
		  int row, int col)
{
	/*
	 * FIXME make sure there is not already a piece on this position.
	 * FIXME also make sure we don't call it with bad row/col values.
	 */
	uint64_t bb = ((uint64_t)1) << (row * 8 + col);
	FC_BITBOARD((*board), player, piece) |= bb;
	return 1;
}

int fc_get_piece (fc_board_t *board,
		  fc_player_t *player,
		  fc_piece_t *piece,
		  int row, int col)
{
	uint64_t bb = ((uint64_t)1) << (row * 8 + col);
	/* NOTE: I'm using 24 below instead of FC_TOTAL_BITBOARDS because I
	 * only want to look at the bitboards that represent pieces */
	for (int i = 0; i < 24; i++) {
		if ((*board)[i] & bb) {
			*player = i / 6;
			*piece = i % 6;
			return 1;
		}
	}
	return 0;
}

/*
 * The format of a config file is:
 *     1 N c3
 *     3 K h8
 * etc.
 *
 * TODO make this function prettier and more robust.  For example, currently
 * two different pieces are allowed to occupy the same space; we should
 * probably fail if that is the case.  Also think about adding errno.
 */
int fc_setup_board (fc_board_t *board, const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		return 0;
	}

	int player;
	char piece, col, row;
	int read = fscanf(fp, "%d %c %c%c \n", &player, &piece, &col, &row);
	while (read != EOF) {
		if (read != 4) {
			fclose(fp);
			return 0;
		}

		if ((col < 'a' || col > 'h') || (row < '1' || row > '8')) {
			fclose(fp);
			return 0;
		}

		if (player < 1 || player > 4) {
			fclose(fp);
			return 0;
		}
		player -= 1;

		fc_piece_t p;
		switch (piece) {
		case 'P':
			p = FC_PAWN; break;
		case 'B':
			p = FC_BISHOP; break;
		case 'N':
			p = FC_KNIGHT; break;
		case 'R':
			p = FC_ROOK; break;
		case 'Q':
			p = FC_QUEEN; break;
		case 'K':
			p = FC_KING; break;
		default:
			fclose(fp);
			return 0;
		}

		fc_set_piece(board, player, p, row - '1', col - 'a');
		read = fscanf(fp, "%d %c %c%c \n", &player, &piece, &col, &row);
	}
	fclose(fp);
	return 1;
}

#define FC_ALL_PIECES(b, p) \
	(b[p] | b[p+1] | b[p+2] | b[p+3] | b[p+4] | b[p+5])

#define FC_ALL_ALLIES(board, player) \
	(FC_ALL_PIECES(board, player) | FC_ALL_PIECES(board, (player + 2) % 4))

/*
 * Simply determines if the space 'm' is empty or occupied by a piece belonging
 * to a friendly ally.
 */
static inline int may_capture (fc_board_t *b, fc_player_t p, uint64_t m)
{
	return !(m & FC_ALL_ALLIES((*b), p));
}

#define FC_LEFT_COL  (UINT64_C(0x0101010101010101))
#define FC_RIGHT_COL (UINT64_C(0x8080808080808080))

/* assuming there is only one king per player */
int fc_get_king_moves (fc_board_t *board, fc_mlist_t *moves, fc_player_t player)
{
	uint64_t king = FC_BITBOARD((*board), player, FC_KING);
	if (!king) {
		return 0;
	}

	int i = 0;
	uint64_t space;
	if (!(king & FC_LEFT_COL)) {
		space = king << 7;
		if (space && may_capture(board, player, space)) {
			fc_mlist_append(moves, player, FC_KING, king | space);
		}
		space = king >> 1;
		if (may_capture(board, player, space)) {
			fc_mlist_append(moves, player, FC_KING, king | space);
		}
		space = king >> 9;
		if (space && may_capture(board, player, space)) {
			fc_mlist_append(moves, player, FC_KING, king | space);
		}
	}
	space = king << 8;
	if (space && may_capture(board, player, space)) {
		fc_mlist_append(moves, player, FC_KING, king | space);
	}
	space = king >> 8;
	if (space && may_capture(board, player, space)) {
		fc_mlist_append(moves, player, FC_KING, king | space);
	}
	if (!(king & FC_RIGHT_COL)) {
		space = king << 9;
		if (space && may_capture(board, player, space)) {
			fc_mlist_append(moves, player, FC_KING, king | space);
		}
		space = king << 1;
		if (may_capture(board, player, space)) {
			fc_mlist_append(moves, player, FC_KING, king | space);
		}
		space = king >> 7;
		if (space && may_capture(board, player, space)) {
			fc_mlist_append(moves, player, FC_KING, king | space);
		}
	}
	return 1;
}

#define FC_2LEFT_COL  (UINT64_C(0x0202020202020202))
#define FC_2RIGHT_COL (UINT64_C(0x4040404040404040))

int fc_get_knight_moves (fc_board_t *board,
			 fc_mlist_t *moves,
			 fc_player_t player)
{
	/* TODO this whole function */
}

int fc_get_moves (fc_board_t *board, fc_mlist_t *moves, fc_player_t player)
{
	return fc_get_king_moves(board, moves, player);
}
