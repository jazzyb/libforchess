/* FIXME clean up the comments in here with doxygen */
#ifndef _FC_BOARD_H_
#define _FC_BOARD_H_

#include <stdint.h>
#include <stdio.h>

#include "forchess/moves.h"

/*
 * Why 24?  Because we have to keep track of 6 different pieces for 4 players.
 * NOTE:  We will need to change how we organize the board_t type if we decide
 * to optimize by adding more bitboards.
 */
#define FC_TOTAL_BITBOARDS 24

typedef uint64_t fc_board_t[FC_TOTAL_BITBOARDS];

/*
 * Initializes the board based on a config file.
 */
int fc_setup_board (fc_board_t *board, const char *filename);

/*
 * Places the given piece on the board.
 */
int fc_set_piece (fc_board_t *board,
		  fc_player_t player,
		  fc_piece_t piece,
		  int row, int col);

/*
 * Sets the value of the player and piece to that of the piece at the given
 * location.
 */
int fc_get_piece (fc_board_t *board,
		  fc_player_t *player,
		  fc_piece_t *piece,
		  int row, int col);
/* TODO fix the code for the move list structure */
int fc_get_moves (fc_board_t *board, fc_mlist_t *moves, fc_player_t player);
/* other possible functions for the API
int fc_get_removes (fc_board_t *board,
		    fc_player_t player,
		    fc_move_t *moves,
		    int moves_len);
int fc_make_move (fc_board_t *board, fc_move_t *move);
#define FC_CHECK 1
#define FC_CHECKMATE 2
int fc_is_king_in_check (fc_board_t *board, fc_player_t player);

int fc_ai_get_move (fc_board_t *board,
		    fc_move_t *move,
		    fc_player_t player,
		    int look_ahead);
*/

#endif
