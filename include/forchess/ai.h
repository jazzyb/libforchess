/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FC_AI_H_
#define _FC_AI_H_

/**
 * @file forchess/ai.h
 * @author jazzyb aka Jason M Barnes
 * @brief API for the LibForchess AI code.
 */

#ifndef DOXYGEN_IGNORE
#include <time.h>

#include "forchess/board.h"

typedef struct {
	fc_board_t *board;
	fc_board_t *bv; /* board vector */
	fc_mlist_t *mlv; /* move list vector */
	time_t timeout;
} fc_ai_t;

#endif /* DOXYGEN_IGNORE */

/**
 * @brief Initializes the AI structure.
 *
 * Sets the given board for the AI and initializes the default piece material
 * values.
 *
 * @param[in,out] ai A pointer to the AI structure.
 * @param[in,out] ai A pointer to the game board.
 *
 * @return void
 */
void fc_ai_init (fc_ai_t *ai, fc_board_t *board);

/**
 * @brief Returns the best move as determined by the AI.
 *
 * Looks depth moves ahead for player.  Sets the move variable to the best
 * move.  The parameter move should be allocated before the call to this
 * function.
 *
 * @param[in] board A pointer to the game board.
 * @param[out] move The (returned) best move.
 * @param[in] player The player we are finding the best move for.
 * @param[in] depth Number of moves to look ahead.  Each player's move
 * represents one "move".  So to look ahead one whole turn, you will need to
 * pass in 4.
 * @param[in] seconds Approximate amount of time to spend looking for a move.
 * If seconds is 0, then no time limit is placed on the search; otherwise, the
 * search will return as quickly as possible after the number of seconds have
 * expired.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_ai_next_move (fc_ai_t *ai, fc_move_t *move, fc_player_t player,
		int depth, unsigned int seconds, size_t num_threads);

/**
 * @brief Determine the relative material worth of a given board configuration.
 *
 * Returns the player's "score".  This number is determined by adding up
 * player's and player's partner's material score and subtracting their
 * opponent's material score.  This gives a basic idea of who is ahead (not
 * taking into account position) in the game.
 *
 * @param[in] board A pointer to the game board.
 * @param[in] player The player we are returning the score for.
 *
 * @return The relative material score as defined above.
 */
int fc_ai_score_position (fc_ai_t *ai, fc_player_t player);

/**
 * @brief Determines whether or not the given move will put the player's or
 * partner's king in check.
 *
 * A "valid" move meets the following criteria:
 * 	-# It does not move the player's king into check.
 * 	-# It does not put our partner's king into check (unless he is already
 * 	in check).
 * 	-# If the player's king is in check, then it will move him out of
 * 	check...
 * 	-# unless he's in checkmate; in which case the player may move any
 * 	piece EXCEPT the king.
 *
 * @param[in] board A pointer to the game board.
 * @param[in] move The move we are testing.
 *
 * @return 1 if the move is "valid" as defined above; 0 otherwise
 */
int fc_ai_is_move_valid (fc_board_t *board, fc_move_t *move);

#endif