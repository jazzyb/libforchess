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

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

#include "forchess/ai.h"
#include "forchess/board.h"
#include "forchess/moves.h"

void fc_ai_init (fc_ai_t *ai, fc_board_t *board)
{
	assert(ai && board);
	ai->board = board;
	ai->bv = NULL;
	ai->mlv = NULL;
	ai->algo = FC_NEGASCOUT;
}

void fc_ai_set_algorithm (fc_ai_t *ai, fc_ai_algo_t algo)
{
	assert(ai);
	ai->algo = algo;
}

static int time_up (fc_ai_t *ai)
{
	if (ai->timeout == 0) {
		return 0;
	}

	return time(NULL) >= ai->timeout;
}

/*
 * Used by init_move_iterator if we have already calculated a list of valid
 * moves, and we just want to return them one at a time.
 */
static fc_move_t *return_move (void *dummy, fc_mlist_t *list, int *index)
{
	return fc_mlist_get(list, *index);
}

static void init_move_iterator (fc_mlist_iter_t *iter, fc_mlist_t *given,
		fc_board_state_t *state, fc_mlist_t *list, fc_board_t *board,
		fc_player_t player, int depth)
{
	if (given) {
		/*
		 * If we are searching many turns ahead (like, say, 3), it
		 * will help to go ahead and search, e.g., 2 turns ahead, and
		 * then use that list of moves as a starting point to search
		 * deeper.  The hypothesis is that a single turn shouldn't
		 * change the ranking of moves by too much most of the time.
		 * Initial experiments have seemed to confirm this.
		 */
		fc_mlist_iter_init(iter, given, state, return_move);
	} else {
		fc_board_get_all_moves(board, list, player);
		fc_board_state_init(state, board, player);
		fc_mlist_iter_init(iter, list, state, fc_board_get_next_move);
	}
}

/*
 * Adjusts the alpha and beta values given the score.  If ret is not NULL, it
 * copies the move to ret.  Returns 1 if the given score was a cutoff for the
 * search; 0 otherwise.
 */
static int alphabeta_cutoff (int score, int *alpha, int *beta, int max)
{
	if (max && score > *alpha) {
		*alpha = score;
	} else if (!max && score < *beta) {
		*beta = score;
	}
	if (*beta <= *alpha) {
		return 1;
	}

	return 0;
}

/*
 * Returns the value of the subtree.  If the variable max is set to 1, then the
 * function will try to maximize the value.  If max is set to 0, then it will
 * try to minimize the value.
 *
 * If ret is !NULL, then ret will be set to the move with the best score.
 */
static int alphabeta (fc_ai_t *ai, fc_mlist_t *ret, fc_mlist_t *given,
		fc_player_t player, int depth, int alpha, int beta, int max)
{
	int score;
	fc_board_t *board, *copy;
	fc_board_state_t state;
	fc_move_t *move;
	fc_mlist_t *list;
	fc_mlist_iter_t iter;

	if (time_up(ai)) {
		/*
		 * Return a value that will fail the conditions in
		 * alphabeta_cutoff(), i.e. don't take this move into
		 * consideration.
		 */
		return (max) ? beta : alpha;
	}
	board = &(ai->bv[depth]);
	if (fc_board_game_over(board) || depth == 0) {
		score = fc_board_score_position(board, player);
		/*
		 * Adjusting the scores with the current depth expedites the
		 * end of the game.  Otherwise the AI will just move from one
		 * position to the next without making the killing blow.
		 */
		return (max) ? score - depth : (-1 * score) + depth;
	}
	if (fc_board_is_player_out(board, player)) {
		return alphabeta(ai, NULL, NULL, FC_NEXT_PLAYER(player), depth,
				alpha, beta, !max);
	}

	copy = &(ai->bv[depth - 1]);
	list = &(ai->mlv[depth - 1]);
	fc_mlist_clear(list);
	init_move_iterator(&iter, given, &state, list, board, player, depth);
	while ((move = fc_mlist_iter_next(&iter)) != NULL) {
		fc_board_copy(copy, board);
		fc_board_make_move(copy, move);

		score = alphabeta(ai, NULL, NULL, FC_NEXT_PLAYER(player),
				depth - 1, alpha, beta, !max);

		if (ret) {
			fc_mlist_insert(ret, move, score);
		}

		if (alphabeta_cutoff(score, &alpha, &beta, max)) {
			break;
		}
	}

	/* insert the remaining moves if any onto the end of the list */
	if (ret) {
		while ((move = fc_mlist_iter_next(&iter)) != NULL) {
			fc_mlist_insert(ret, move, INT_MIN);
		}
	}

	return (max) ? alpha : beta;
}

/*
 * There is no difference between the negascout cutoff condition and the
 * *maximizing* alphabeta cutoff condition.
 */
static int negascout_cutoff (int score, int *alpha, int *beta)
{
	return alphabeta_cutoff(score, alpha, beta, 1);
}

static int negascout (fc_ai_t *ai, fc_mlist_t *ret, fc_mlist_t *given,
		fc_player_t player, int depth, int alpha, int beta)
{
	int b, first, score;
	fc_board_t *board, *copy;
	fc_board_state_t state;
	fc_move_t *move;
	fc_mlist_t *list;
	fc_mlist_iter_t iter;

	if (time_up(ai)) {
		return beta;
	}
	board = &(ai->bv[depth]);
	if (fc_board_game_over(board) || depth == 0) {
		score = fc_board_score_position(board, player);
		return score;
	}
	if (fc_board_is_player_out(board, player)) {
		return -negascout(ai, NULL, NULL, FC_NEXT_PLAYER(player),
				depth, -beta, -alpha);
	}

	copy = &(ai->bv[depth - 1]);
	list = &(ai->mlv[depth - 1]);
	fc_mlist_clear(list);
	init_move_iterator(&iter, given, &state, list, board, player, depth);
	b = beta;
	first = 1;
	while ((move = fc_mlist_iter_next(&iter)) != NULL) {
		fc_board_copy(copy, board);
		fc_board_make_move(copy, move);

		score = -negascout(ai, NULL, NULL, FC_NEXT_PLAYER(player),
				depth - 1, -b, -alpha);

		if (!first && alpha < score && score < beta) {
			score = -negascout(ai, NULL, NULL,
					FC_NEXT_PLAYER(player), depth - 1,
					-beta, -alpha);
		}
		first = 0;

		if (ret) {
			fc_mlist_insert(ret, move, score);
		}

		if (negascout_cutoff(score, &alpha, &beta)) {
			break;
		}

		b = alpha + 1;
	}

	if (ret) {
		while ((move = fc_mlist_iter_next(&iter)) != NULL) {
			fc_mlist_insert(ret, move, INT_MIN);
		}
	}

	return alpha;
}

/*
 * Sets the parameter ret to the best move based on alphabeta pruning of the
 * minmax game tree.
 */
int fc_ai_next_move (fc_ai_t *ai, fc_move_t *ret, fc_mlist_t *given,
		fc_player_t player, int depth, unsigned int seconds)
{
	int rc;
	fc_mlist_t list;

	fc_mlist_init(&list);
	rc = fc_ai_next_ranked_moves(ai, &list, given, player, depth, seconds);
	if (ret) {
		fc_move_copy(ret, fc_mlist_get(&list, 0));
	}
	fc_mlist_free(&list);
	return rc;
}

static void free_ai_mlists (fc_ai_t *ai, int depth)
{
	int i;

	for (i = 0; i < depth; i++) {
		fc_mlist_free(&(ai->mlv[i]));
	}
	free(ai->mlv);
	ai->mlv = NULL;
}

static void initialize_ai_mlists (fc_ai_t *ai, int depth)
{
	int i;

	if (ai->mlv != NULL) {
		free_ai_mlists(ai, depth);
	}
	ai->mlv = calloc(depth, sizeof(fc_mlist_t));
	for (i = 0; i < depth; i++) {
		fc_mlist_init(&(ai->mlv[i]));
	}
}

static void free_ai_boards (fc_ai_t *ai)
{
	free(ai->bv);
	ai->bv = NULL;
}

static void initialize_ai_boards (fc_ai_t *ai, int depth)
{
	if (ai->bv != NULL) {
		free_ai_boards(ai);
	}
	ai->bv = calloc(depth + 1, sizeof(fc_board_t));
	fc_board_copy(&(ai->bv[depth]), ai->board);
}

#define ALPHA_MIN INT_MIN
#define BETA_MAX INT_MAX

int fc_ai_next_ranked_moves (fc_ai_t *ai, fc_mlist_t *ret, fc_mlist_t *given,
		fc_player_t player, int depth, unsigned int seconds)
{
	assert(ai && ai->board && ret);
	if (fc_board_is_player_out(ai->board, player) || depth < 1) {
		return 0;
	}

	initialize_ai_mlists(ai, depth);
	initialize_ai_boards(ai, depth);
	ai->timeout = (seconds) ? time(NULL) + seconds : 0;

	switch (ai->algo) {
	case FC_ALPHABETA:
		alphabeta(ai, ret, given, player, depth, ALPHA_MIN, BETA_MAX,
				1);
		break;
	case FC_NEGASCOUT:
		negascout(ai, ret, given, player, depth, ALPHA_MIN + 1,
				BETA_MAX);
		break;
	default:
		assert(0);
	}

	free_ai_boards(ai);
	free_ai_mlists(ai, depth);

	return 1;
}

