#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "forchess/board.h"

/* FIXME switch the row/col values to x/y ones; the current way seems backwards
 * */
int fc_board_set_piece (fc_board_t *board,
			fc_player_t player,
			fc_piece_t piece,
			int row, int col)
{
	/*
	 * FIXME make sure there is not already a piece on this position.
	 * FIXME also make sure we don't call it with bad row/col values.
	 */
	uint64_t bb = ((uint64_t)1) << (row * 8 + col);
	FC_BITBOARD((*board), player, piece) |= bb;
	if (piece == FC_PAWN) {
		FC_PAWN_BB((*board), player) |= bb;
	}
	return 1;
}

/*
 * Returns 1 and sets the values of player and piece if it found a piece at the
 * given row and col.  Returns 0 if the space was empty.
 */
int fc_board_get_piece (fc_board_t *board,
			fc_player_t *player,
			fc_piece_t *piece,
			int row, int col)
{
	uint64_t bb = ((uint64_t)1) << (row * 8 + col);
	/* NOTE: I'm using 24 below instead of FC_TOTAL_BITBOARDS because I
	 * only want to look at the bitboards that represent pieces */
	for (int i = 0; i < 24; i++) {
		if ((*board)[i] & bb) {
			*player = i / 6;
			*piece = i % 6;
			return 1;
		}
	}
	return 0;
}

/*
 * Removes any piece at the given row and column.
 * Returns 0 if there was no piece at the given coordinates; 1 otherwise.
 */
int fc_board_remove_piece (fc_board_t *board, int row, int col)
{
	fc_player_t player;
	fc_piece_t piece;
	fc_board_get_piece(board, &player, &piece, row, col);
	uint64_t bit = UINT64_C(1) << (row * 8 + col);
	for (int i = 0; i < 24; i++) {
		if ((*board)[i] & bit) {
			if (piece == FC_PAWN) {
				FC_PAWN_BB((*board), player) ^= bit;
			}
			(*board)[i] ^= bit;
			return 1;
		}
	}
	return 0;
}

/*
 * The format of a config file is:
 *     1 N c3
 *     3 K h8
 * etc.
 *
 * TODO make this function prettier and more robust.  For example, currently
 * two different pieces are allowed to occupy the same space; we should
 * probably fail if that is the case.  Also think about adding errno.
 */
int fc_board_setup (fc_board_t *board, const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		return 0;
	}

	int player;
	char piece, col, row;
	int read = fscanf(fp, "%d %c %c%c \n", &player, &piece, &col, &row);
	while (read != EOF) {
		if (read != 4) {
			fclose(fp);
			return 0;
		}

		if ((col < 'a' || col > 'h') || (row < '1' || row > '8')) {
			fclose(fp);
			return 0;
		}

		if (player < 1 || player > 4) {
			fclose(fp);
			return 0;
		}
		player -= 1;

		fc_piece_t p;
		switch (piece) {
		case 'P':
			p = FC_PAWN; break;
		case 'B':
			p = FC_BISHOP; break;
		case 'N':
			p = FC_KNIGHT; break;
		case 'R':
			p = FC_ROOK; break;
		case 'Q':
			p = FC_QUEEN; break;
		case 'K':
			p = FC_KING; break;
		default:
			fclose(fp);
			return 0;
		}

		fc_board_set_piece(board, player, p, row - '1', col - 'a');
		read = fscanf(fp, "%d %c %c%c \n", &player, &piece, &col, &row);
	}
	fclose(fp);
	return 1;
}

/*
 * Simply determines if the space 'm' is empty or occupied by a piece belonging
 * to a friendly ally.
 */
static inline int may_move_to (fc_board_t *b, fc_player_t p, uint64_t m)
{
	return !(m & FC_ALL_ALLIES((*b), p));
}

/*
 * This function updates the move list with the new move if the piece in
 * question is allowed to move to the given space.
 *
 * NOTE: This function is only used for king and knight; the corresponding
 * function for pawns is called pawn_move_if_valid() and move_and_continue()
 * for bishop, rook, and queen.
 */
static inline void move_if_valid (fc_board_t *board,
				  fc_mlist_t *moves,
				  fc_player_t player,
				  fc_piece_t type,
				  uint64_t piece,
				  uint64_t space)
{
	if (space && may_move_to(board, player, space)) {
		fc_mlist_append(moves, player, type, FC_NONE, piece | space);
	}
}

/* assuming there is only one king per player */
void fc_get_king_moves (fc_board_t *board, fc_mlist_t *moves, fc_player_t player)
{
	uint64_t king = FC_BITBOARD((*board), player, FC_KING);
	if (!king) {
		return;
	}

	if (!(king & FC_LEFT_COL)) {
		move_if_valid(board, moves, player, FC_KING, king, king << 7);
		move_if_valid(board, moves, player, FC_KING, king, king >> 1);
		move_if_valid(board, moves, player, FC_KING, king, king >> 9);
	}

	move_if_valid(board, moves, player, FC_KING, king, king << 8);
	move_if_valid(board, moves, player, FC_KING, king, king >> 8);

	if (!(king & FC_RIGHT_COL)) {
		move_if_valid(board, moves, player, FC_KING, king, king << 9);
		move_if_valid(board, moves, player, FC_KING, king, king << 1);
		move_if_valid(board, moves, player, FC_KING, king, king >> 7);
	}
}

void fc_get_knight_moves (fc_board_t *board,
			 fc_mlist_t *moves,
			 fc_player_t player)
{
	uint64_t knight, bb = FC_BITBOARD((*board), player, FC_KNIGHT);
	if (!bb) {
		return;
	}

	FC_FOREACH(knight, bb) {
		if (!(knight & (FC_LEFT_COL | FC_2LEFT_COL))) {
			move_if_valid(board, moves, player, FC_KNIGHT, knight,
				      knight << 6);
			move_if_valid(board, moves, player, FC_KNIGHT, knight,
				      knight >> 10);
		}
		if (!(knight & FC_LEFT_COL)) {
			move_if_valid(board, moves, player, FC_KNIGHT, knight,
				      knight << 15);
			move_if_valid(board, moves, player, FC_KNIGHT, knight,
				      knight >> 17);
		}
		if (!(knight & (FC_RIGHT_COL | FC_2RIGHT_COL))) {
			move_if_valid(board, moves, player, FC_KNIGHT, knight,
				      knight << 10);
			move_if_valid(board, moves, player, FC_KNIGHT, knight,
				      knight >> 6);
		}
		if (!(knight & FC_RIGHT_COL)) {
			move_if_valid(board, moves, player, FC_KNIGHT, knight,
				      knight << 17);
			move_if_valid(board, moves, player, FC_KNIGHT, knight,
				      knight >> 15);
		}
	}
}

/*
 * NOTE: I am utilizing the double-!'s below because if I simply return the
 * value, then the int will truncate the uint64_t resulting in false negatives.
 * The double-! forces the value to be 0 or 1.
 */
static inline int is_occupied_by_enemy (fc_board_t *b,
					fc_player_t p,
					uint64_t m)
{
	return !!(m & FC_ALL_ALLIES((*b), ((p + 1) % 4)));
}

static inline int is_empty (fc_board_t *b, uint64_t m)
{
	return !!(m & ~(FC_ALL_PIECES((*b), 0) | FC_ALL_PIECES((*b), 1) |
			FC_ALL_PIECES((*b), 2) | FC_ALL_PIECES((*b), 3)));
}

/*
 * Basically the same as move_if_valid() above with the following changes:
 * 	m1 represents the single available pawn diagonal movement
 * 	m2 and m3 are the lateral capture moves
 */
static inline void pawn_move_if_valid (fc_board_t *board,
				       fc_mlist_t *moves,
				       fc_player_t player,
				       uint64_t pawn,
				       uint64_t m1,
				       uint64_t m2,
				       uint64_t m3)
{
	if (is_empty(board, m1)) {
		fc_mlist_append(moves, player, FC_PAWN, FC_NONE, pawn | m1);
	}

	if (is_occupied_by_enemy(board, player, m2)) {
		fc_mlist_append(moves, player, FC_PAWN, FC_NONE, pawn | m2);
	}
	if (is_occupied_by_enemy(board, player, m3)) {
		fc_mlist_append(moves, player, FC_PAWN, FC_NONE, pawn | m3);
	}
}

static inline fc_player_t fc_get_pawn_orientation (fc_board_t *board,
						   uint64_t pawn)
{
	if (pawn & (*board)[FC_FIRST_PAWNS]) {
		return FC_FIRST;
	} else if (pawn & (*board)[FC_SECOND_PAWNS]) {
		return FC_SECOND;
	} else if (pawn & (*board)[FC_THIRD_PAWNS]) {
		return FC_THIRD;
	} else if (pawn & (*board)[FC_FOURTH_PAWNS]) {
		return FC_FOURTH;
	} else {
		return FC_NONE;
	}
}

/*
 * NOTE:  I am not doing the regular checks to determine if the pawn is on the
 * edge of the board because in any normal game the pawn will be promoted once
 * it reaches the edge; this means that you could setup a board arrangement
 * that would return invalid moves for pawns or potentially crash this API.
 * Don't do that.
 */
void fc_get_pawn_moves (fc_board_t *board,
			fc_mlist_t *moves,
			fc_player_t player)
{
	uint64_t pawn, bb = FC_BITBOARD((*board), player, FC_PAWN);
	if (!bb) {
		return;
	}

	FC_FOREACH(pawn, bb) {
		switch (fc_get_pawn_orientation(board, pawn)) {
		case FC_FIRST:
			pawn_move_if_valid(board, moves, player, pawn,
					   pawn << 9, pawn << 8, pawn << 1);
			break;
		case FC_SECOND:
			pawn_move_if_valid(board, moves, player, pawn,
					   pawn >> 7, pawn >> 8, pawn << 1);
			break;
		case FC_THIRD:
			pawn_move_if_valid(board, moves, player, pawn,
					   pawn >> 9, pawn >> 8, pawn >> 1);
			break;
		case FC_FOURTH:
			pawn_move_if_valid(board, moves, player, pawn,
					   pawn << 7, pawn << 8, pawn >> 1);
			break;
		}
	}
}

/*
 * This function appends a move to the mlist if piece is allowed to move to
 * space.  Returns 1 if the calling function may continue evaluating moves
 * (i.e. the space is empty); 0 otherwise.
 */
static inline int move_and_continue (fc_board_t *board,
					  fc_mlist_t *moves,
					  fc_player_t player,
					  fc_piece_t type,
					  uint64_t piece,
					  uint64_t space)
{
	if (is_empty(board, space)) {
		fc_mlist_append(moves, player, type, FC_NONE, piece | space);
		return 1;
	} else if (is_occupied_by_enemy(board, player, space)) {
		fc_mlist_append(moves, player, type, FC_NONE, piece | space);
	}
	return 0;
}

static void fc_get_northwest_moves (fc_board_t *board,
				    fc_mlist_t *moves,
				    fc_player_t player,
				    fc_piece_t type,
				    uint64_t piece)
{
	if (piece & FC_LEFT_COL) {
		return;
	}

	for (uint64_t i = piece << 7; i; i <<= 7) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_LEFT_COL) {
			break;
		}
	}
}

static void fc_get_southwest_moves (fc_board_t *board,
				    fc_mlist_t *moves,
				    fc_player_t player,
				    fc_piece_t type,
				    uint64_t piece)
{
	if (piece & FC_LEFT_COL) {
		return;
	}

	for (uint64_t i = piece >> 9; i; i >>= 9) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_LEFT_COL) {
			break;
		}
	}
}

static void fc_get_northeast_moves (fc_board_t *board,
				    fc_mlist_t *moves,
				    fc_player_t player,
				    fc_piece_t type,
				    uint64_t piece)
{
	if (piece & FC_RIGHT_COL) {
		return;
	}

	for (uint64_t i = piece << 9; i; i <<= 9) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_RIGHT_COL) {
			break;
		}
	}
}

static void fc_get_southeast_moves (fc_board_t *board,
				    fc_mlist_t *moves,
				    fc_player_t player,
				    fc_piece_t type,
				    uint64_t piece)
{
	if (piece & FC_RIGHT_COL) {
		return;
	}

	for (uint64_t i = piece >> 7; i; i >>= 7) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_RIGHT_COL) {
			break;
		}
	}
}

void fc_get_bishop_moves (fc_board_t *board,
			  fc_mlist_t *moves,
			  fc_player_t player)
{
	uint64_t bishop, bb = FC_BITBOARD((*board), player, FC_BISHOP);
	if (!bb) {
		return;
	}

	FC_FOREACH(bishop, bb) {
		fc_get_northwest_moves(board, moves, player, FC_BISHOP, bishop);
		fc_get_southwest_moves(board, moves, player, FC_BISHOP, bishop);
		fc_get_northeast_moves(board, moves, player, FC_BISHOP, bishop);
		fc_get_southeast_moves(board, moves, player, FC_BISHOP, bishop);
	}
}

static void fc_get_upward_moves (fc_board_t *board,
				 fc_mlist_t *moves,
				 fc_player_t player,
				 fc_piece_t type,
				 uint64_t piece)
{
	for (uint64_t i = piece << 8; i; i <<= 8) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}
	}
}

static void fc_get_downward_moves (fc_board_t *board,
				   fc_mlist_t *moves,
				   fc_player_t player,
				   fc_piece_t type,
				   uint64_t piece)
{
	for (uint64_t i = piece >> 8; i; i >>= 8) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}
	}
}

static void fc_get_leftward_moves (fc_board_t *board,
				   fc_mlist_t *moves,
				   fc_player_t player,
				   fc_piece_t type,
				   uint64_t piece)
{
	if (piece & FC_LEFT_COL) {
		return;
	}

	for (uint64_t i = piece >> 1; i; i >>= 1) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_LEFT_COL) {
			break;
		}
	}
}

static void fc_get_rightward_moves (fc_board_t *board,
				    fc_mlist_t *moves,
				    fc_player_t player,
				    fc_piece_t type,
				    uint64_t piece)
{
	if (piece & FC_RIGHT_COL) {
		return;
	}

	for (uint64_t i = piece << 1; i; i <<= 1) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_RIGHT_COL) {
			break;
		}
	}
}

void fc_get_rook_moves (fc_board_t *board,
			fc_mlist_t *moves,
			fc_player_t player)
{
	uint64_t rook, bb = FC_BITBOARD((*board), player, FC_ROOK);
	if (!bb) {
		return;
	}

	FC_FOREACH(rook, bb) {
		fc_get_upward_moves(board, moves, player, FC_ROOK, rook);
		fc_get_downward_moves(board, moves, player, FC_ROOK, rook);
		fc_get_leftward_moves(board, moves, player, FC_ROOK, rook);
		fc_get_rightward_moves(board, moves, player, FC_ROOK, rook);
	}
}

void fc_get_queen_moves (fc_board_t *board,
			 fc_mlist_t *moves,
			 fc_player_t player)
{
	uint64_t queen, bb = FC_BITBOARD((*board), player, FC_QUEEN);
	if (!bb) {
		return;
	}

	FC_FOREACH(queen, bb) {
		fc_get_upward_moves(board, moves, player, FC_QUEEN, queen);
		fc_get_downward_moves(board, moves, player, FC_QUEEN, queen);
		fc_get_leftward_moves(board, moves, player, FC_QUEEN, queen);
		fc_get_rightward_moves(board, moves, player, FC_QUEEN, queen);
		fc_get_northwest_moves(board, moves, player, FC_QUEEN, queen);
		fc_get_southwest_moves(board, moves, player, FC_QUEEN, queen);
		fc_get_northeast_moves(board, moves, player, FC_QUEEN, queen);
		fc_get_southeast_moves(board, moves, player, FC_QUEEN, queen);
	}
}

void fc_board_get_moves (fc_board_t *board,
			 fc_mlist_t *moves,
			 fc_player_t player)
{
	fc_get_pawn_moves(board, moves, player);
	fc_get_knight_moves(board, moves, player);
	fc_get_bishop_moves(board, moves, player);
	fc_get_rook_moves(board, moves, player);
	fc_get_queen_moves(board, moves, player);
	fc_get_king_moves(board, moves, player);
}

/*
 * Return the pieces that the player is allowed to remove (i.e. all the pieces
 * the player has).
 */
void fc_board_get_removes (fc_board_t *board,
			   fc_mlist_t *moves,
			   fc_player_t player)
{
	for (fc_piece_t type = FC_PAWN; type <= FC_KING; type++) {
		uint64_t piece, bb = FC_BITBOARD((*board), player, type);
		FC_FOREACH(piece, bb) {
			fc_mlist_append(moves, player, type, FC_NONE, piece);
		}
	}
}

/*
 * Give all player 'from's pieces to player 'to'.
 */
static void fc_convert_pieces (fc_board_t *board,
			       fc_player_t from,
			       fc_player_t to)
{
	uint64_t pawns = FC_BITBOARD((*board), from, FC_PAWN);
	for (fc_bitboards_t i = FC_FIRST_PAWNS; i <= FC_FOURTH_PAWNS; i++) {
		if (pawns & (*board)[i]) {
			FC_BITBOARD((*board), to, FC_PAWN) |= (*board)[i];
		}
	}
	FC_BITBOARD((*board), from, FC_PAWN) = UINT64_C(0);

	for (fc_piece_t i = FC_PAWN + 1; i < FC_KING; i++) {
		FC_BITBOARD((*board), to, i) |= FC_BITBOARD((*board), from, i);
		FC_BITBOARD((*board), from, i) = UINT64_C(0);
	}
}

/*
 * If the given pawn reaches its "back wall" it will get promoted.
 */
#define FC_FIRST_BACK_WALL  UINT64_C(0xff80808080808080)
#define FC_SECOND_BACK_WALL UINT64_C(0x80808080808080ff)
#define FC_THIRD_BACK_WALL  UINT64_C(0x01010101010101ff)
#define FC_FOURTH_BACK_WALL UINT64_C(0xff01010101010101)
static inline int must_promote (fc_player_t player, uint64_t pawn)
{
	switch (player) {
	case FC_FIRST:
		return !!(pawn & FC_FIRST_BACK_WALL);
	case FC_SECOND:
		return !!(pawn & FC_SECOND_BACK_WALL);
	case FC_THIRD:
		return !!(pawn & FC_THIRD_BACK_WALL);
	case FC_FOURTH:
		return !!(pawn & FC_FOURTH_BACK_WALL);
	default:
		assert(0);
	}
}

/*
 * Find the piece bitboard that needs to be updated for the enemy of player.
 *
 * Cycle through the bitboards of all our enemies assuming that we
 * aren't trying to capture our own allies.
 */
static int update_enemy_bitboards (fc_board_t *board, fc_player_t player,
		fc_player_t enemy, fc_player_t enemy_side, uint64_t bit)
{
	for (fc_piece_t type = FC_PAWN; type <= FC_KING; type++) {
		if (bit & FC_BITBOARD((*board), enemy, type)) {
			FC_BITBOARD((*board), enemy, type) ^= bit;
			if (type == FC_PAWN) {
				assert(enemy_side != FC_NONE);
				FC_PAWN_BB((*board), enemy_side) ^= bit;
			}
			if (type == FC_KING) {
				fc_convert_pieces(board, enemy, player);
			}
			return 1;
		}
	}
	return 0;
}

/*
 * Update the board with the given move.  Returns 0 iff there is no piece
 * specified in the case of a pawn promotion; returns 1 otherwise.
 */
int fc_board_make_move (fc_board_t *board, fc_move_t *move)
{
	fc_player_t side;
	if (move->piece == FC_PAWN) {
		/* If necessary, handle pawn promotions. */
		uint64_t pawn = (FC_BITBOARD((*board), move->player, FC_PAWN) ^
				move->move) & move->move;
		side = fc_get_pawn_orientation(board, pawn ^ move->move);
		if (must_promote(side, pawn)) {
			if (move->promote == FC_NONE) {
				return 0;
			} else {
				return fc_board_make_pawn_move(board, move,
						move->promote);
			}
		}
	}

	/*
	 * Move the player's piece; then get the second bit (b) that represents
	 * the possible captured piece.
	 */
	FC_BITBOARD((*board), move->player, move->piece) ^= move->move;
	uint64_t b = FC_BITBOARD((*board), move->player, move->piece) &
		     move->move;

	/* update pawn orientation bitboards */
	fc_player_t enemy_side = fc_get_pawn_orientation(board, b);
	if (move->piece == FC_PAWN) {
		side = fc_get_pawn_orientation(board, b ^ move->move);
		FC_PAWN_BB((*board), side) ^= move->move;
	}

	/*
	 * If there is no second bit, then this was only a remove, and we can
	 * return.
	 */
	if (!b) {
		return 1;
	}

	if (!update_enemy_bitboards(board, move->player, (move->player + 1) % 4,
				enemy_side, b)) {
		(void)update_enemy_bitboards(board, move->player,
				(move->player + 3) % 4, enemy_side, b);
	}

	return 1;
}

/*
 * If a pawn is moved in such a way that it must be promoted, then
 * fc_board_make_move() will return 0.  In that case, the user must call the
 * below function with a third argument declaring what piece the pawn should be
 * promoted to.
 */
int fc_board_make_pawn_move (fc_board_t *board,
			     fc_move_t *move,
			     fc_piece_t new_piece)
{
	if (move->piece != FC_PAWN) {
		return 0;
	}
	uint64_t pawn = FC_BITBOARD((*board), move->player,
				    FC_PAWN) & move->move;

	switch(new_piece) {
	case FC_BISHOP:
	case FC_KNIGHT:
	case FC_ROOK:
	case FC_QUEEN:
		FC_BITBOARD((*board), move->player, new_piece) |= pawn;
		break;
	default:
		return 0;
	}
	FC_BITBOARD((*board), move->player, FC_PAWN) ^= pawn;
	fc_player_t orientation = fc_get_pawn_orientation(board, pawn);
	FC_PAWN_BB((*board), orientation) ^= pawn;

	fc_move_t copy;
	fc_move_copy(&copy, move);
	copy.piece = new_piece;
	return fc_board_make_move(board, &copy);
}

void fc_board_copy (fc_board_t *dst, fc_board_t *src)
{
	for (int i = 0; i < FC_TOTAL_BITBOARDS; i++) {
		(*dst)[i] = (*src)[i];
	}
}
