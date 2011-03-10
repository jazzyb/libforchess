#ifndef _FC_AI_H_
#define _FC_AI_H_

#include <limits.h>

#include "forchess/board.h"

#define ALPHA_MIN INT_MIN
#define BETA_MAX INT_MAX

int fc_next_best_move (fc_board_t *board, fc_move_t *move, fc_player_t player);

#endif
