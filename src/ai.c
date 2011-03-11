#include <limits.h>

/* FIXME write tests for the below code.
 * FIXME fill in incomplete sections.
 * FIXME add logic to remove pieces as well as move them.
 */

#include "forchess/ai.h"
#include "forchess/board.h"
#include "forchess/move.h"

/*
 * Return 1 if the game is won; 0 otherwise.
 */
static inline int game_over (fc_board_t *board, fc_player_t player)
{
}

/*
 * Return 1 if player is no longer present in the game; 0 otherwise.
 */
static inline int is_player_out (fc_board_t *board, fc_player_t player)
{
}

#define FC_NEXT_PLAYER(player) ((player + 1) % 4)
/*
 * Returns the value of the subtree.  If the variable max is set to 1, then the
 * function will try to maximize the value.  If max is set to 0, then it will
 * try to minimize the value.
 */
static int alphabeta (fc_board_t *board, fc_player_t player, int depth,
		int alpha, int beta, int max)
{
	int score;
	if (game_over(board, player) || depth == 0) {
		score = fc_ai_score_position(board, player);
		return (max) ? score : (-1 * score);
	}
	if (is_player_out(board, player)) {
		return alphabeta(board, FC_NEXT_PLAYER(player), depth, alpha,
				beta, !max);
	}

	fc_mlist_t list;
	fc_mlist_init(&list, 0);
	fc_board_get_moves(board, &list, player);
	for (int i = 0; i < fc_mlist_length(&list); i++) {

		fc_move_t *move = fc_mlist_get(list, i);
		if (!fc_ai_is_move_valid(board, move)) {
			continue;
		}

		fc_board_t copy;
		fc_board_copy(&copy, board);
		fc_board_make_move(&copy, move);

		score = alphabeta(&copy, FC_NEXT_PLAYER(player), depth - 1,
				alpha, beta, !max);

		if (max) {
			if (score > alpha) {
				alpha = score;
			}
		} else {
			if (score < beta) {
				beta = score;
			}
		}
		if (beta <= alpha) {
			break;
		}
	}
	fc_mlist_free(&list);

	return (max) ? alpha : beta;
}

#define ALPHA_MIN INT_MIN
#define BETA_MAX INT_MAX
/*
 * Sets the parameter ret to the best move based on alphabeta pruning of the
 * minmax game tree.
 */
int fc_ai_next_move (fc_board_t *board, fc_move_t *ret, fc_player_t player,
		int depth)
{
	/* TODO check that this request is valid */
	int alpha = ALPHA_MIN;
	fc_mlist_t list;
	fc_mlist_init(&list, 0);
	fc_board_get_moves(board, &list, player);
	for (int i = 0; i < fc_mlist_length(&list); i++) {

		fc_move_t *move = fc_mlist_get(&list, i);
		if (!fc_ai_is_move_valid(board, move)) {
			continue;
		}

		fc_board_t copy;
		fc_board_copy(&copy, board);
		fc_board_make_move(&copy, move);

		int score = alphabeta(&copy, FC_NEXT_PLAYER(player), depth - 1,
				alpha, BETA_MAX, 1);
		if (score > alpha) {
			alpha = value;
			fc_move_copy(ret, move);
		}
	}
	fc_mlist_free(&list);
	return 1;
}

int fc_ai_score_position (fc_board_t *board, fc_player_t player)
{
}

int fc_ai_is_move_valid (fc_board_t *board, fc_move_t *move)
{
}
