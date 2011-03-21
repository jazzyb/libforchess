#ifndef _FC_AI_H_
#define _FC_AI_H_

#include "forchess/board.h"

int fc_ai_next_move (fc_board_t *board, fc_move_t *move, fc_player_t player,
		int depth);
int fc_ai_score_position (fc_board_t *board, fc_player_t player);
int fc_ai_is_move_valid (fc_board_t *board, fc_move_t *move);

#endif
