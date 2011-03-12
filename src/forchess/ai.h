#ifndef _FC_AI_H_
#define _FC_AI_H_

#include "forchess/board.h"

int _fc_ai_piece_values[] = {
	100,	/* pawns */
	300,	/* bishops */
	350,	/* knights -- because of the closeness of the pieces, I value
		   them slightly more than bishops */
	500,	/* rooks */
	900,	/* queens */
	100000	/* kings */
}

int fc_ai_next_move (fc_board_t *board, fc_move_t *move, fc_player_t player,
		int depth);
int fc_ai_score_position (fc_board_t *board, fc_player_t player);
int fc_ai_is_move_valid (fc_board_t *board, fc_move_t *move);

#endif
