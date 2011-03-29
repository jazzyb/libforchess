/* FIXME clean up the comments in here with doxygen */
#ifndef _FC_BOARD_H_
#define _FC_BOARD_H_

#include <stdint.h>
#include <stdio.h>

#include "forchess/moves.h"

/*
 * Why 24?  Because we have to keep track of 6 different pieces for 4 players.
 */
typedef enum {
	FC_FIRST_PAWNS = 24,
	FC_SECOND_PAWNS,
	FC_THIRD_PAWNS,
	FC_FOURTH_PAWNS,

	/* This must always be last. */
	FC_TOTAL_BITBOARDS
} fc_bitboards_t;

typedef uint64_t fc_board_t[FC_TOTAL_BITBOARDS];

/* macro to get the first 24 bitboards representing pieces */
#define FC_BITBOARD(board, player, piece) (board[player * 6 + piece])

/*
 * Cycles through each piece (bit) on the bitboard.
 * piece is the bit
 * x is a bitboard
 */
#define FC_FOREACH(piece, x) \
	for(piece = (x & (~x + 1)); x; x ^= piece, piece = (x & (~x + 1)))

/* macro to get a particular pawn orientation bitboard */
#define FC_PAWN_BB(board, orientation) (board[FC_FIRST_PAWNS + orientation])

/* used to check a piece's position on the board */
#define FC_LEFT_COL  (UINT64_C(0x0101010101010101))
#define FC_RIGHT_COL (UINT64_C(0x8080808080808080))
#define FC_2LEFT_COL  (UINT64_C(0x0202020202020202))
#define FC_2RIGHT_COL (UINT64_C(0x4040404040404040))

#define FC_ALL_PIECES(b, p) \
	(b[6*p] | b[6*p+1] | b[6*p+2] | b[6*p+3] | b[6*p+4] | b[6*p+5])

#define FC_ALL_ALLIES(b, p) \
	(FC_ALL_PIECES(b, p) | FC_ALL_PIECES(b, ((p + 2) % 4)))

int is_empty (fc_board_t *board, uint64_t bit);

/*
 * Initializes the board based on a config file.
 */
int fc_board_setup (fc_board_t *board, const char *filename,
		fc_player_t *first);

/*
 * Places the given piece on the board.
 */
int fc_board_set_piece (fc_board_t *board,
			fc_player_t player,
			fc_piece_t piece,
			int row, int col);

/*
 * Sets the value of the player and piece to that of the piece at the given
 * location.
 */
int fc_board_get_piece (fc_board_t *board,
		  fc_player_t *player,
		  fc_piece_t *piece,
		  int row, int col);
int fc_board_remove_piece (fc_board_t *board, int row, int col);
/* NOTE because there is no return value for this function, if one of the mlist
 * functions fails, the user will not know about it. */
void fc_board_get_moves (fc_board_t *board,
			 fc_mlist_t *moves,
			 fc_player_t player);
void fc_board_get_removes (fc_board_t *board,
			  fc_mlist_t *moves,
			  fc_player_t player);
int fc_board_make_move (fc_board_t *board, fc_move_t *move);
int fc_board_make_pawn_move (fc_board_t *board,
			     fc_move_t *move,
			     fc_piece_t promote);
void fc_board_copy (fc_board_t *dst, fc_board_t *src);
#define FC_CHECK 1
#define FC_CHECKMATE 2
int fc_board_check_status (fc_board_t *board, fc_player_t player);

#endif
