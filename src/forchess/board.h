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

/*
 * Initializes the board based on a config file.
 */
int fc_board_setup (fc_board_t *board, const char *filename);

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
/* other possible functions for the API
#define FC_CHECK 1
#define FC_CHECKMATE 2
int fc_is_king_in_check (fc_board_t *board, fc_player_t player);

int fc_ai_get_move (fc_board_t *board,
		    fc_move_t *move,
		    fc_player_t player,
		    int look_ahead);
*/

#endif
