/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 */

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "forchess/ai.h"
#include "forchess/board.h"
#include "forchess/moves.h"

void fc_ai_init (fc_ai_t *ai, fc_board_t *board)
{
	assert(ai && board);
	ai->board = board;
	ai->bv = NULL;
	ai->mlv = NULL;
}

/*
 * Return 1 if player is no longer present in the game; 0 otherwise.
 */
static int is_player_out (fc_board_t *board, fc_player_t player)
{
	return !(FC_BITBOARD(board, player, FC_KING));
}

/*
 * Return 1 if one side has no remaining moves; 0 otherwise.
 */
static int game_over (fc_board_t *board)
{
	return ((is_player_out(board, FC_FIRST) &&
		is_player_out(board, FC_THIRD)) ||
		(is_player_out(board, FC_SECOND) &&
		is_player_out(board, FC_FOURTH)));
}

static void append_pawn_promotions_to_moves(fc_board_t *board, fc_mlist_t *list,
		fc_move_t *move)
{
	move->promote = FC_QUEEN;
	board->list_add_move(list, move);
	move->promote = FC_KNIGHT;
	board->list_add_move(list, move);
	move->promote = FC_ROOK;
	board->list_add_move(list, move);
	move->promote = FC_BISHOP;
	board->list_add_move(list, move);
	move->promote = FC_NONE;
}

/*
 * All of the code below was once a part of fc_ai_is_move_valid() but was
 * pulled out to increase the speed of the alphabeta function.  See the comment
 * above fc_ai_is_move_valid() for an explanation of what this function is
 * looking for.
 */
static int is_move_valid_given_check_status (fc_board_t *board, fc_move_t *move,
		int check_status_before, int partner_status_before)
{
	int check_status_after, partner_status_after;
	uint64_t king;
	fc_board_t copy;

	fc_board_copy(&copy, board);
	fc_board_make_move(&copy, move);

	if (check_status_before == FC_CHECKMATE) {
		king = FC_BITBOARD(board, move->player, FC_KING);
		if (move->piece == FC_KING && move->move != king) {
			return 0;
		} else {
			return 1;
		}
	}
	check_status_after = fc_board_check_status(&copy, move->player);
	if (!check_status_before && check_status_after) {
		return 0;
	}
	if (check_status_before == FC_CHECK && check_status_after) {
		return 0;
	}

	partner_status_after = fc_board_check_status(&copy,
			FC_PARTNER(move->player));
	if (!partner_status_before && partner_status_after) {
		return 0;
	}

	return 1;
}

static void move_and_adjust_scores (fc_move_t *mv, fc_ai_t *ai,
		fc_move_t *ret, fc_player_t player, int depth, int *alpha,
		int *beta, int max);

static int alphabeta_handle_removes(fc_ai_t *ai, fc_move_t *ret,
		fc_player_t player, int depth, int alpha, int beta, int max);

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
	int current_check_status, partner_check_status;
	int all_moves_are_invalid;
	fc_board_t *orig, *board;
	fc_mlist_t *list;
	fc_move_t *move;
	fc_player_t dummy;

	board = &(ai->bv[depth]);
	if (game_over(board) || depth == 0) {
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
	if (is_player_out(board, player)) {
		return alphabeta(ai, NULL, FC_NEXT_PLAYER(player), depth,
				alpha, beta, !max);
	}

	list = &(ai->mlv[depth - 1]);
	fc_mlist_clear(list);
	fc_board_get_moves(board, list, player);

	all_moves_are_invalid = 1;
	current_check_status = fc_board_check_status(board, player);
	partner_check_status = fc_board_check_status(board,
			FC_PARTNER(player));
	for (i = 0; i < fc_mlist_length(list); i++) {

		move = fc_mlist_get(list, i);
		if (!is_move_valid_given_check_status(board, move,
					current_check_status,
					partner_check_status)) {
			continue;
		}

		if (fc_board_move_requires_promotion(board, move, &dummy) &&
				move->promote == FC_NONE) {
			append_pawn_promotions_to_moves(board, list, move);
			continue;
		}
		all_moves_are_invalid = 0;

		move_and_adjust_scores(move, ai, ret, player, depth, &alpha,
				&beta, max);

		if (beta <= alpha) {
			break;
		}
	}

	if (all_moves_are_invalid) {
		return alphabeta_handle_removes(ai, ret, player, depth, alpha,
				beta, max);
	}

	return (max) ? alpha : beta;
}

/*
 * Used in alphabeta() and alphabeta_handle_removes().  Makes the given move on
 * the board and gets the material score of the board.  Adjusts alpha and beta
 * if necessary and sets the ret move pointer to the best move if ret != NULL.
 */
static void move_and_adjust_scores (fc_move_t *mv, fc_ai_t *ai,
		fc_move_t *ret, fc_player_t player, int depth, int *alpha,
		int *beta, int max)
{
	int score;
	fc_board_t *board, *copy;

	board = &(ai->bv[depth]);
	copy = &(ai->bv[depth - 1]);
	fc_board_copy(copy, board);
	fc_board_make_move(copy, mv);
	score = alphabeta(ai, NULL, FC_NEXT_PLAYER(player), depth - 1, *alpha,
			*beta, !max);

	if (max && score > *alpha) {
		*alpha = score;
		if (ret) {
			fc_move_copy(ret, mv);
		}
	} else if (!max && score < *beta) {
		*beta = score;
		if (ret) {
			fc_move_copy(ret, mv);
		}
	}
}

/*
 * Same as alphabeta above, but evaluates the removes for a position instead of
 * the moves.  This function is only called from the end of alphabeta(), so we
 * can assume that all the checks for game_over(), etc. have already been done.
 */
static int alphabeta_handle_removes(fc_ai_t *ai, fc_move_t *ret,
		fc_player_t player, int depth, int alpha, int beta, int max)
{
	int i;
	int found_valid_move = 0;
	fc_board_t *board;
	fc_mlist_t *list;
	fc_move_t *rm;

	board = &(ai->bv[depth]);
	list = &(ai->mlv[depth - 1]);
	fc_mlist_clear(list);
	fc_board_get_removes(board, list, player);
	/*
	 * Go through the list the first time.  If we can remove a piece other
	 * than the king that won't put us in check, great.
	 */
	for (i = 0; i < fc_mlist_length(list); i++) {
		rm = fc_mlist_get(list, i);
		if (rm->piece == FC_KING || !fc_ai_is_move_valid(board, rm)) {
			continue;
		}
		found_valid_move = 1;

		move_and_adjust_scores(rm, ai, ret, player, depth, &alpha,
				&beta, max);

		if (beta <= alpha) {
			break;
		}
	}
	if (found_valid_move) {
		return (max) ? alpha : beta;
	}

	/*
	 * If we only have the king to remove, then remove it.
	 */
	if (fc_mlist_length(list) == 1) {
		move_and_adjust_scores(fc_mlist_get(list, 0), ai, ret, player,
				depth, &alpha, &beta, max);

		return (max) ? alpha : beta;
	}

	/*
	 * Otherwise, go through the whole list again (this time allowing
	 * putting kings into check(mate)). And return the best move.
	 */
	for (i = 0; i < fc_mlist_length(list); i++) {
		rm = fc_mlist_get(list, i);
		if (rm->piece == FC_KING) {
			continue;
		}

		move_and_adjust_scores(rm, ai, ret, player, depth, &alpha,
				&beta, max);

		if (beta <= alpha) {
			break;
		}
	}

	return (max) ? alpha : beta;
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
		fc_mlist_init(&(ai->mlv[i]), 0);
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
/*
 * Sets the parameter ret to the best move based on alphabeta pruning of the
 * minmax game tree.
 */
int fc_ai_next_move (fc_ai_t *ai, fc_move_t *ret, fc_player_t player, int depth)
{
	assert(ai && ai->board && ret);
	if (is_player_out(ai->board, player) || depth < 1) {
		ret->move = 0;
		return 0;
	}

	initialize_ai_mlists(ai, depth);
	initialize_ai_boards(ai, depth);

	(void)alphabeta(ai, ret, player, depth, ALPHA_MIN, BETA_MAX, 1);

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

/*
 * Verifies that we are:
 * 	1. Not moving our king into check.
 * 	2. Not putting our partner's king into check.
 * 	3. If our king is in check, then moving him out of check...
 * 	4. ...unless he's in checkmate; in which case we can move anything BUT
 * 	   the king.
 *
 * Returns 1 if all of the above are true (i.e. the move is allowed); 0
 * otherwise.
 *
 * See is_move_valid_given_check_status() above.
 */
int fc_ai_is_move_valid (fc_board_t *board, fc_move_t *move)
{
	int check_status_before, partner_status_before;

	assert(board && move);
	check_status_before = fc_board_check_status(board, move->player);
	partner_status_before = fc_board_check_status(board,
			FC_PARTNER(move->player));
	return is_move_valid_given_check_status(board, move,
			check_status_before, partner_status_before);
}

