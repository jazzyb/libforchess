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
#include "forchess/threads.h"

void fc_ai_init (fc_ai_t *ai, fc_board_t *board)
{
	assert(ai && board);
	ai->board = board;
	ai->bv = NULL;
	ai->mlv = NULL;
}

static int time_up (fc_ai_t *ai)
{
	if (ai->timeout == 0) {
		return 0;
	}

	return time(NULL) >= ai->timeout;
}

/*
 * Adjusts the alpha and beta values given the score.  If ret is not NULL, it
 * copies the move to ret.  Returns 1 if the given score was a cutoff for the
 * search; 0 otherwise.
 */
static int alphabeta_cutoff (int score, int *alpha, int *beta, fc_move_t *move,
		fc_move_t *ret, int max)
{
	if (max && score > *alpha) {
		*alpha = score;
		if (ret) {
			fc_move_copy(ret, move);
		}
	} else if (!max && score < *beta) {
		*beta = score;
		if (ret) {
			fc_move_copy(ret, move);
		}
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
static int alphabeta (fc_ai_t *ai, fc_move_t *ret, fc_player_t player,
		int depth, int alpha, int beta, int max)
{
	int i;
	int score;
	fc_board_t *orig, *board, *copy;
	fc_mlist_t *list;
	fc_move_t *move;

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
		orig = ai->board;
		ai->board = board;
		score = fc_ai_score_position(ai, player);
		ai->board = orig;
		/*
		 * Adjusting the scores with the current depth expedites the
		 * end of the game.  Otherwise the AI will just move from one
		 * position to the next without making the killing blow.
		 */
		return (max) ? score - depth : (-1 * score) + depth;
	}
	if (fc_board_is_player_out(board, player)) {
		return alphabeta(ai, NULL, FC_NEXT_PLAYER(player), depth,
				alpha, beta, !max);
	}

	copy = &(ai->bv[depth - 1]);
	list = &(ai->mlv[depth - 1]);
	fc_mlist_clear(list);
	fc_board_get_moves(board, list, player);
	for (i = 0; i < fc_mlist_length(list); i++) {

		move = fc_mlist_get(list, i);
		fc_board_copy(copy, board);
		fc_board_make_move(copy, move);

		score = alphabeta(ai, NULL, FC_NEXT_PLAYER(player), depth - 1,
				alpha, beta, !max);

		if (alphabeta_cutoff(score, &alpha, &beta, move, ret, max)) {
			break;
		}
	}

	return (max) ? alpha : beta;
}

static int negascout (fc_ai_t *ai, fc_move_t *ret, fc_player_t player,
		int depth, int alpha, int beta)
{
	int i, b;
	int score;
	fc_board_t *board, *copy;
	fc_mlist_t *list;
	fc_move_t *move;

	if (time_up(ai)) {
		return beta;
	}
	board = &(ai->bv[depth]);
	if (fc_board_game_over(board) || depth == 0) {
		fc_board_t *orig = ai->board;
		ai->board = board;
		score = fc_ai_score_position(ai, player);
		ai->board = orig;
		return score;
	}
	if (fc_board_is_player_out(board, player)) {
		return -negascout(ai, NULL, FC_NEXT_PLAYER(player), depth,
				-beta, -alpha);
	}

	copy = &(ai->bv[depth - 1]);
	list = &(ai->mlv[depth - 1]);
	fc_mlist_clear(list);
	fc_board_get_moves(board, list, player);
	for (b = beta, i = 0; i < fc_mlist_length(list); b = alpha + 1, i++) {

		move = fc_mlist_get(list, i);
		fc_board_copy(copy, board);
		fc_board_make_move(copy, move);

		score = -negascout(ai, NULL, FC_NEXT_PLAYER(player), depth - 1,
				-b, -alpha);

		if (i != 0 && alpha < score && score < beta) {
			score = -negascout(ai, NULL, FC_NEXT_PLAYER(player),
					depth - 1, -beta, -alpha);
		}

		if (score > alpha) {
			alpha = score;
			if (ret) {
				fc_move_copy(ret, move);
			}
		}

		if (alpha >= beta) {
			break;
		}
	}

	return alpha;
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

struct ab_input {
	fc_board_t board;
	fc_player_t player;
	int depth;
	time_t timeout;
	int alpha;
	int beta;
	int max;
};

struct ab_output {
	int score;
	fc_move_t *move;
};

void fc_ai_alphabeta_wrapper (void *input, void *output)
{
	fc_ai_t ai;
	struct ab_input *in = input;
	struct ab_output *out = output;

	fc_ai_init(&ai, &in->board);
	initialize_ai_mlists(&ai, in->depth);
	initialize_ai_boards(&ai, in->depth);
	ai.timeout = in->timeout;

	out->score = alphabeta(&ai, NULL, in->player, in->depth, in->alpha,
			in->beta, in->max);

	free_ai_boards(&ai);
	free_ai_mlists(&ai, in->depth);
}

static int threaded_move_search (fc_ai_t *ai, fc_tpool_t *pool,
		fc_move_t *ret, fc_player_t player, int depth, int alpha,
		int beta, int max)
{
	int i, rc, count, score;
	struct ab_input *inputs;
	struct ab_output *outputs;
	struct ab_output *retval;
	fc_mlist_t list;
	fc_move_t *move;
	fc_board_t *orig, *copy, *board = &(ai->bv[depth]);

	if (fc_board_is_player_out(board, player)) {
		return threaded_move_search(ai, pool, NULL,
				FC_NEXT_PLAYER(player), depth, alpha, beta,
				!max);
	}

	if (fc_board_game_over(board) || depth == 0) {
		orig = ai->board;
		ai->board = board;
		score = fc_ai_score_position(ai, player);
		ai->board = orig;
		/* FIXME:  Why is this different than alphabeta above? */
		return (max) ? score + depth : (-1 * score) - depth;
	}

	/* set alpha and beta based on the initial move */
	copy = &(ai->bv[depth - 1]);
	fc_board_copy(copy, board);
	fc_mlist_init(&list);
	fc_board_get_moves(board, &list, player);
	move = fc_mlist_get(&list, 0);
	fc_board_make_move(copy, move);

	score = threaded_move_search(ai, pool, NULL, FC_NEXT_PLAYER(player),
			depth - 1, alpha, beta, !max);

	if (alphabeta_cutoff(score, &alpha, &beta, move, ret, max)) {
		return (max) ? alpha : beta;
	}

	/* then go through the rest of the list */
	inputs = calloc(fc_mlist_length(&list), sizeof(*inputs));
	outputs = calloc(fc_mlist_length(&list), sizeof(*outputs));
	for (i = 1, count = 0; i < fc_mlist_length(&list); i++) {
		fc_board_copy(&inputs[i].board, board);
		outputs[i].move = fc_mlist_get(&list, i);
		fc_board_make_move(&inputs[i].board, outputs[i].move);
		inputs[i].player = FC_NEXT_PLAYER(player);
		inputs[i].depth = depth - 1;
		inputs[i].timeout = ai->timeout;
		inputs[i].alpha = alpha;
		inputs[i].beta = beta;
		inputs[i].max = !max;

		rc = fc_tpool_push_task(pool, fc_ai_alphabeta_wrapper,
				inputs + i, outputs + i);
		assert(rc);
		count += 1;
	}

	while (count) {
		rc = fc_tpool_pop_result(pool, (void **)&retval);
		if (!rc) {
			continue;
		}

		count -= 1;
		if (alphabeta_cutoff(retval->score, &alpha, &beta, retval->move,
					ret, max)) {
			break;
		}
	}

	fc_tpool_clear_tasks(pool);
	free(inputs);
	free(outputs);
	fc_mlist_free(&list);
	return (max) ? alpha : beta;
}

#define ALPHA_MIN INT_MIN
#define BETA_MAX INT_MAX
/*
 * Sets the parameter ret to the best move based on alphabeta pruning of the
 * minmax game tree.
 */
int fc_ai_next_move (fc_ai_t *ai, fc_move_t *ret, fc_player_t player,
		int depth, unsigned int seconds, size_t num_threads)
{
	fc_tpool_t pool;

	assert(ai && ai->board && ret);
	if (fc_board_is_player_out(ai->board, player) || depth < 1) {
		ret->move = 0;
		return 0;
	}

	initialize_ai_mlists(ai, depth);
	initialize_ai_boards(ai, depth);
	ai->timeout = (seconds) ? time(NULL) + seconds : 0;

	if (num_threads <= 1) {
		negascout(ai, ret, player, depth, ALPHA_MIN + 1, BETA_MAX);
	} else {
		fc_tpool_init(&pool, num_threads, FC_DEFAULT_MLIST_SIZE);
		fc_tpool_start_threads(&pool);
		threaded_move_search(ai, &pool, ret, player, depth, ALPHA_MIN,
				BETA_MAX, 1);
		fc_tpool_stop_threads(&pool);
		fc_tpool_free(&pool);
	}

	free_ai_boards(ai);
	free_ai_mlists(ai, depth);

	return 1;
}

static int get_material_score (fc_ai_t *ai, fc_player_t player)
{
	int ret = 0;
	uint64_t piece, pieces;
	fc_piece_t i;

	for (i = FC_PAWN; i <= FC_KING; i++) {
		pieces = FC_BITBOARD(ai->board, player, i);
		FC_FOREACH(piece, pieces) {
			ret += fc_board_get_material_value(ai->board, i);
		}
	}
	return ret;
}

int fc_ai_score_position (fc_ai_t *ai, fc_player_t player)
{
	assert(ai);
	return (get_material_score(ai, player) -
		get_material_score(ai, FC_NEXT_PLAYER(player)) +
		get_material_score(ai, FC_PARTNER(player)) -
		get_material_score(ai, FC_PARTNER(FC_NEXT_PLAYER(player))));
}

