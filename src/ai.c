#include <limits.h>

/* FIXME write tests for the below code.
 * FIXME add logic to remove pieces as well as move them.
 * FIXME add logic to handle pawn promotions
 */

#include "forchess/ai.h"
#include "forchess/board.h"
#include "forchess/moves.h"

int _fc_ai_piece_values[] = {
	100,	/* pawns */
	300,	/* bishops */
	350,	/* knights -- because of the closeness of the pieces, I value
		   them slightly more than bishops */
	500,	/* rooks */
	900,	/* queens */
	100000	/* kings */
};

/*
 * Return 1 if player is no longer present in the game; 0 otherwise.
 */
static inline int is_player_out (fc_board_t *board, fc_player_t player)
{
	return !FC_BITBOARD((*board), player, FC_KING);
}

/*
 * Return 1 if one side has no remaining moves; 0 otherwise.
 */
static inline int game_over (fc_board_t *board)
{
	return ((is_player_out(board, FC_FIRST) &&
		is_player_out(board, FC_THIRD)) ||
		(is_player_out(board, FC_SECOND) &&
		is_player_out(board, FC_FOURTH)));
}

/*
 * Returns the value of the subtree.  If the variable max is set to 1, then the
 * function will try to maximize the value.  If max is set to 0, then it will
 * try to minimize the value.
 */
static int alphabeta (fc_board_t *board, fc_player_t player, int depth,
		int alpha, int beta, int max)
{
	int score;
	if (game_over(board) || depth == 0) {
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
	/* TODO add logic for removes */
	/* TODO add logic for those weird times when we won't be able to move,
	 * but we will have to remove a piece that will put us in check(mate).
	 */
	for (int i = 0; i < fc_mlist_length(&list); i++) {

		fc_move_t *move = fc_mlist_get(&list, i);
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
				alpha, BETA_MAX, 0);
		if (score > alpha) {
			alpha = score;
			fc_move_copy(ret, move);
		}
	}
	fc_mlist_free(&list);
	return 1;
}

static int get_material_score (fc_board_t *board, fc_player_t player)
{
	int ret = 0;
	for (fc_piece_t i = FC_PAWN; i <= FC_KING; i++) {
		uint64_t piece, pieces = FC_BITBOARD((*board), player, i);
		FC_FOREACH(piece, pieces) {
			ret += _fc_ai_piece_values[i];
		}
	}
	return ret;
}

int fc_ai_score_position (fc_board_t *board, fc_player_t player)
{
	return (get_material_score(board, player) -
		get_material_score(board, FC_NEXT_PLAYER(player)) +
		get_material_score(board, FC_PARTNER(player)) -
		get_material_score(board, FC_PARTNER(FC_NEXT_PLAYER(player))));
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
 */
int fc_ai_is_move_valid (fc_board_t *board, fc_move_t *move)
{
	fc_board_t copy;
	fc_board_copy(&copy, board);
	fc_board_make_move(&copy, move);

	int check_status_before = fc_is_king_in_check(board, move->player);
	if (check_status_before == FC_CHECKMATE) {
		if (move->piece == FC_KING) {
			return 0;
		} else {
			return 1;
		}
	}
	int check_status_after = fc_is_king_in_check(&copy, move->player);
	if (!check_status_before && check_status_after) {
		return 0;
	}
	if (check_status_before == FC_CHECK && check_status_after) {
		return 0;
	}

	int partner_status_before = fc_is_king_in_check(board,
			FC_PARTNER(move->player));
	int partner_status_after = fc_is_king_in_check(&copy,
			FC_PARTNER(move->player));
	if (!partner_status_before && partner_status_after) {
		return 0;
	}

	return 1;
}
