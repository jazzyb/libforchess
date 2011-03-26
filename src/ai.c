#include <assert.h>
#include <limits.h>

#include "forchess/ai.h"
#include "forchess/board.h"
#include "forchess/moves.h"

int _fc_ai_piece_values[] = {
	100,	/* pawns */
	300,	/* bishops */
	350,	/* knights */
	500,	/* rooks */
	900,	/* queens */
	100000	/* kings */
};

/*
 * Return 1 if player is no longer present in the game; 0 otherwise.
 */
static inline int is_player_out (fc_board_t *board, fc_player_t player)
{
	return !(FC_BITBOARD((*board), player, FC_KING));
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

static inline void append_pawn_promotions_to_moves(fc_mlist_t *list,
		fc_move_t *move)
{
	fc_mlist_append(list, move->player, move->piece, FC_BISHOP, move->move);
	fc_mlist_append(list, move->player, move->piece, FC_KNIGHT, move->move);
	fc_mlist_append(list, move->player, move->piece, FC_ROOK, move->move);
	fc_mlist_append(list, move->player, move->piece, FC_QUEEN, move->move);
}

#define BAD_MOVES 0xdeadbeef /* an unlikely score */
/*
 * Returns the value of the subtree.  If the variable max is set to 1, then the
 * function will try to maximize the value.  If max is set to 0, then it will
 * try to minimize the value.
 *
 * If ret is !NULL, then ret will be set to the move with the best score.
 */
static int alphabeta (fc_board_t *board, fc_move_t *ret, fc_player_t player,
		int depth, int alpha, int beta, int max)
{
	int score = BAD_MOVES;
	if (game_over(board) || depth == 0) {
		score = fc_ai_score_position(board, player);
		/*
		 * Adjusting the scores with the current depth expedites the
		 * end of the game.  Otherwise the AI will just move from one
		 * position to the next without making the killing blow.
		 */
		return (max) ? score - depth : (-1 * score) + depth;
	}
	if (is_player_out(board, player)) {
		return alphabeta(board, NULL, FC_NEXT_PLAYER(player), depth,
				alpha, beta, !max);
	}

	fc_mlist_t list;
	fc_mlist_init(&list, 0);
	fc_board_get_moves(board, &list, player);
	/* TODO add logic for those weird times when we won't be able to move,
	 * but we will have to remove a piece that will put us in check(mate).
	 */
evaluate_moves:
	for (int i = 0; i < fc_mlist_length(&list); i++) {

		fc_move_t *move = fc_mlist_get(&list, i);
		if (!fc_ai_is_move_valid(board, move)) {
			continue;
		}

		fc_board_t copy;
		fc_board_copy(&copy, board);
		if (fc_board_make_move(&copy, move)) {
			score = alphabeta(&copy, NULL, FC_NEXT_PLAYER(player),
					depth - 1, alpha, beta, !max);
		} else { /* move requires pawn promotion */
			append_pawn_promotions_to_moves(&list, move);
			continue;
		}

		if (max && score > alpha) {
			alpha = score;
			if (ret) {
				fc_move_copy(ret, move);
			}
		} else if (!max && score < beta) {
			beta = score;
			if (ret) {
				fc_move_copy(ret, move);
			}
		}

		if (beta <= alpha) {
			break;
		}
	}

	if (score == BAD_MOVES) {
		fc_mlist_clear(&list);
		fc_board_get_removes(board, &list, player);
		assert(BAD_MOVES + 1 != score++);
		goto evaluate_moves;
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
	assert(board && ret);
	if (is_player_out(board, player) || depth < 1) {
		ret->move = 0;
		return 0;
	}
	(void)alphabeta(board, ret, player, depth, ALPHA_MIN, BETA_MAX, 1);
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
	assert(board);
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
	assert(board && move);
	fc_board_t copy;
	fc_board_copy(&copy, board);
	fc_board_make_move(&copy, move);

	int check_status_before = fc_board_check_status(board, move->player);
	if (check_status_before == FC_CHECKMATE) {
		uint64_t king = FC_BITBOARD((*board), move->player, FC_KING);
		if (move->piece == FC_KING && move->move != king) {
			return 0;
		} else {
			return 1;
		}
	}
	int check_status_after = fc_board_check_status(&copy, move->player);
	if (!check_status_before && check_status_after) {
		return 0;
	}
	if (check_status_before == FC_CHECK && check_status_after) {
		return 0;
	}

	int partner_status_before = fc_board_check_status(board,
			FC_PARTNER(move->player));
	int partner_status_after = fc_board_check_status(&copy,
			FC_PARTNER(move->player));
	if (!partner_status_before && partner_status_after) {
		return 0;
	}

	return 1;
}
