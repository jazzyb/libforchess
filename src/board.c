#include <stdio.h>
#include <stdlib.h>

#include "forchess/board.h"

/* macro to get a particular pawn orientation bitboard */
#define FC_PAWN_BB(board, orientation) (board[FC_FIRST_PAWNS + orientation])

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
	uint64_t bit = UINT64_C(1) << (row * 8 + col);
	for (int i = 0; i < 24; i++) {
		if ((*board)[i] & bit) {
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

#define FC_ALL_PIECES(b, p) \
	(b[6*p] | b[6*p+1] | b[6*p+2] | b[6*p+3] | b[6*p+4] | b[6*p+5])

#define FC_ALL_ALLIES(b, p) \
	(FC_ALL_PIECES(b, p) | FC_ALL_PIECES(b, ((p + 2) % 4)))

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
		fc_mlist_append(moves, player, type, piece | space);
	}
}

#define FC_LEFT_COL  (UINT64_C(0x0101010101010101))
#define FC_RIGHT_COL (UINT64_C(0x8080808080808080))

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

#define FC_2LEFT_COL  (UINT64_C(0x0202020202020202))
#define FC_2RIGHT_COL (UINT64_C(0x4040404040404040))

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
		fc_mlist_append(moves, player, FC_PAWN, pawn | m1);
	}

	if (is_occupied_by_enemy(board, player, m2)) {
		fc_mlist_append(moves, player, FC_PAWN, pawn | m2);
	}
	if (is_occupied_by_enemy(board, player, m3)) {
		fc_mlist_append(moves, player, FC_PAWN, pawn | m3);
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
	} else {
		return FC_FOURTH;
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
		fc_mlist_append(moves, player, type, piece | space);
		return 1;
	} else if (is_occupied_by_enemy(board, player, space)) {
		fc_mlist_append(moves, player, type, piece | space);
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
	fc_get_king_moves(board, moves, player);
	fc_get_pawn_moves(board, moves, player);
	fc_get_knight_moves(board, moves, player);
	fc_get_bishop_moves(board, moves, player);
	fc_get_rook_moves(board, moves, player);
	fc_get_queen_moves(board, moves, player);
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
			fc_mlist_append(moves, player, type, piece);
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
	}
}

/*
 * Update the board with the given move.
 * FIXME refactor/clean-up
 */
int fc_board_make_move (fc_board_t *board, fc_move_t *move)
{
	if (move->piece == FC_PAWN) {
		uint64_t pawn = (FC_BITBOARD((*board), move->player,
					    FC_PAWN) ^ move->move) &
				move->move;
		if (must_promote(fc_get_pawn_orientation(board, pawn), pawn)) {
			return 0;
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
	fc_player_t side;
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

	/*
	 * Cycle through the bitboards of all our enemies assuming that we
	 * aren't trying to capture our own allies.
	 */
	fc_player_t enemy = (move->player + 1) % 4;
	for (fc_piece_t type = FC_PAWN; type <= FC_KING; type++) {
		if (b & FC_BITBOARD((*board), enemy, type)) {
			FC_BITBOARD((*board), enemy, type) ^= b;
			if (type == FC_PAWN) {
				side = fc_get_pawn_orientation(board, b);
				FC_PAWN_BB((*board), side) ^= b;
			}
			if (type == FC_KING) {
				fc_convert_pieces(board, enemy, move->player);
			}
			return 1;
		}
	}

	enemy = (enemy + 2) % 4;
	for (fc_piece_t type = FC_PAWN; type <= FC_KING; type++) {
		if (b & FC_BITBOARD((*board), enemy, type)) {
			FC_BITBOARD((*board), enemy, type) ^= b;
			if (type == FC_PAWN) {
				side = fc_get_pawn_orientation(board, b);
				FC_PAWN_BB((*board), side) ^= b;
			}
			if (type == FC_KING) {
				fc_convert_pieces(board, enemy, move->player);
			}
			return 1;
		}
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
	/* FIXME don't change the move structure.  We will be using this in the
	 * AI code whiich assumes that the move structs aren't changing.
	 * Change this to make a copy before calling make_move() with the copy.
	 */
	move->piece = new_piece;
	return fc_board_make_move(board, move);
}

void fc_board_copy (fc_board_t *dst, fc_board_t *src)
{
	for (int i = 0; i < FC_TOTAL_BITBOARDS; i++) {
		(*dst)[i] = (*src)[i];
	}
}

/* NOTE everything below here is for determining whether the king is in check
 * or not */
/* FIXME Find a clean way of determining if the king is in check(mate). */

static int is_threatened_by_king (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t enemy_kings;
	enemy_kings = FC_BITBOARD((*board), FC_NEXT_PLAYER(player), FC_KING) |
		FC_BITBOARD((*board), FC_PARTNER(FC_NEXT_PLAYER(player)),
				FC_KING);
	if (!(king & FC_LEFT_COL)) {
		if (((king << 7) & enemy_kings) ||
		    ((king >> 9) & enemy_kings) ||
		    ((king >> 1) & enemy_kings)) {
			return 1;
		}
	}
	if (((king >> 8) & enemy_kings) || ((king << 8) & enemy_kings)) {
		return 1;
	}
	if (!(king & FC_RIGHT_COL)) {
		if (((king << 9) & enemy_kings) ||
		    ((king >> 7) & enemy_kings) ||
		    ((king << 1) & enemy_kings)) {
			return 1;
		}
	}
	return 0;
}

/* TODO double check the below function -- it needs to be PERFECT */
static int is_threatened_by_pawn (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t pawns1, pawns2, pawns3, pawns4;
	switch (player) {
	case FC_FIRST:
		/* player 2 pawns that are originally player 2's pawns */
		pawns2 = FC_BITBOARD((*board), FC_SECOND, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_SECOND);
		/* player 2 pawns that (after TWO king captures) are under
		 * player 4's control */
		pawns2 |= FC_BITBOARD((*board), FC_FOURTH, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_SECOND);
		/* partner pawns which are under enemy control */
		pawns3 = FC_BITBOARD((*board), FC_SECOND, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_THIRD);
		pawns3 |= FC_BITBOARD((*board), FC_FOURTH, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_THIRD);
		/* player 4 pawns that are originally player 4's pawns */
		pawns4 = FC_BITBOARD((*board), FC_FOURTH, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_FOURTH);
		/* player 4 pawns that (after TWO king captures) are under
		 * player 2's control */
		pawns4 |= FC_BITBOARD((*board), FC_SECOND, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_FOURTH);
		/* check left */
		if (!(king & FC_LEFT_COL) && ((king >> 1) & pawns2)) {
			return 1;
		}
		/* check right */
		if (!(king & FC_RIGHT_COL) &&
			(((king << 1) & pawns4) || ((king << 1) & pawns3))) {
			return 1;
		}
		/* check up */
		if (((king << 8) & pawns2) || ((king << 8) & pawns3)) {
			return 1;
		}
		/* check down */
		if ((king >> 8) & pawns4) {
			return 1;
		}
		break;
	case FC_SECOND:
		/* player 1 pawns that are originally player 1's pawns */
		pawns1 = FC_BITBOARD((*board), FC_FIRST, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_FIRST);
		/* player 1 pawns that (after TWO king captures) are under
		 * player 3's control */
		pawns1 |= FC_BITBOARD((*board), FC_THIRD, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_FIRST);
		/* partner pawns which are under enemy control */
		pawns4 = FC_BITBOARD((*board), FC_FIRST, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_FOURTH);
		pawns4 |= FC_BITBOARD((*board), FC_THIRD, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_FOURTH);
		/* player 3 pawns that are originally player 3's pawns */
		pawns3 = FC_BITBOARD((*board), FC_THIRD, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_THIRD);
		/* player 3 pawns that (after TWO king captures) are under
		 * player 1's control */
		pawns3 |= FC_BITBOARD((*board), FC_FIRST, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_THIRD);
		/* check left */
		if (!(king & FC_LEFT_COL) && ((king >> 1) & pawns1)) {
			return 1;
		}
		/* check right */
		if (!(king & FC_RIGHT_COL) &&
			(((king << 1) & pawns3) || ((king << 1) & pawns4))) {
			return 1;
		}
		/* check up */
		if ((king << 8) & pawns3) {
			return 1;
		}
		/* check down */
		if (((king >> 8) & pawns1) || ((king >> 8) & pawns4)) {
			return 1;
		}
		break;
	case FC_THIRD:
		/* player 2 pawns that are originally player 2's pawns */
		pawns2 = FC_BITBOARD((*board), FC_SECOND, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_SECOND);
		/* player 2 pawns that (after TWO king captures) are under
		 * player 4's control */
		pawns2 |= FC_BITBOARD((*board), FC_FOURTH, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_SECOND);
		/* partner pawns which are under enemy control */
		pawns1 = FC_BITBOARD((*board), FC_SECOND, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_FIRST);
		pawns1 |= FC_BITBOARD((*board), FC_FOURTH, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_FIRST);
		/* player 4 pawns that are originally player 4's pawns */
		pawns4 = FC_BITBOARD((*board), FC_FOURTH, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_FOURTH);
		/* player 4 pawns that (after TWO king captures) are under
		 * player 2's control */
		pawns4 |= FC_BITBOARD((*board), FC_SECOND, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_FOURTH);
		/* check left */
		if (!(king & FC_LEFT_COL) &&
			(((king >> 1) & pawns2) || ((king >> 1) & pawns1))) {
			return 1;
		}
		/* check right */
		if (!(king & FC_RIGHT_COL) && ((king << 1) & pawns4)) {
			return 1;
		}
		/* check up */
		if ((king << 8) & pawns2) {
			return 1;
		}
		/* check down */
		if (((king >> 8) & pawns4) || ((king >> 8) & pawns1)) {
			return 1;
		}
		break;
	case FC_FOURTH:
		/* player 1 pawns that are originally player 1's pawns */
		pawns1 = FC_BITBOARD((*board), FC_FIRST, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_FIRST);
		/* player 1 pawns that (after TWO king captures) are under
		 * player 3's control */
		pawns1 |= FC_BITBOARD((*board), FC_THIRD, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_FIRST);
		/* partner pawns which are under enemy control */
		pawns2 = FC_BITBOARD((*board), FC_FIRST, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_SECOND);
		pawns2 |= FC_BITBOARD((*board), FC_THIRD, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_SECOND);
		/* player 3 pawns that are originally player 3's pawns */
		pawns3 = FC_BITBOARD((*board), FC_THIRD, FC_PAWN) &
			 FC_PAWN_BB((*board), FC_THIRD);
		/* player 3 pawns that (after TWO king captures) are under
		 * player 1's control */
		pawns3 |= FC_BITBOARD((*board), FC_FIRST, FC_PAWN) &
			  FC_PAWN_BB((*board), FC_THIRD);
		/* check left */
		if (!(king & FC_LEFT_COL) &&
			(((king >> 1) & pawns1) || ((king >> 1) & pawns2))) {
			return 1;
		}
		/* check right */
		if (!(king & FC_RIGHT_COL) && ((king << 1) & pawns3)) {
			return 1;
		}
		/* check up */
		if (((king << 8) & pawns3) || ((king << 8) & pawns2)) {
			return 1;
		}
		/* check down */
		if ((king >> 8) & pawns1) {
			return 1;
		}
		break;
	}
	return 0;
}

static int king_in_check_upward (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	for (uint64_t i = king << 8; i; i <<= 8) {
		if (i & threats) {
			return 1;
		}
		if (i & FC_ALL_ALLIES((*board), player)) {
			break;
		}
	}
	return 0;
}

static int king_in_check_downward (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	for (uint64_t i = king >> 8; i; i >>= 8) {
		if (i & threats) {
			return 1;
		}
		if (i & FC_ALL_ALLIES((*board), player)) {
			break;
		}
	}
	return 0;
}

static int king_in_check_leftward (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	if (king & FC_LEFT_COL) {
		return 0;
	}

	for (uint64_t i = king >> 1; i; i >>= 1) {
		if (i & threats) {
			return 1;
		}
		if (i & FC_ALL_ALLIES((*board), player)) {
			break;
		}

		if (i & FC_LEFT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_rightward (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	if (king & FC_RIGHT_COL) {
		return 0;
	}

	for (uint64_t i = king << 1; i; i <<= 1) {
		if (i & threats) {
			return 1;
		}
		if (i & FC_ALL_ALLIES((*board), player)) {
			break;
		}

		if (i & FC_RIGHT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_laterally (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t threats = FC_BITBOARD((*board), FC_NEXT_PLAYER(player),
			FC_QUEEN);
	threats |= FC_BITBOARD((*board), FC_NEXT_PLAYER(player), FC_ROOK);
	threats |= FC_BITBOARD((*board), FC_PARTNER(FC_NEXT_PLAYER(player)),
			FC_QUEEN);
	threats |= FC_BITBOARD((*board), FC_PARTNER(FC_NEXT_PLAYER(player)),
			FC_ROOK);

	return !!(king_in_check_upward(board, player, king, threats) ||
		  king_in_check_downward(board, player, king, threats) ||
		  king_in_check_leftward(board, player, king, threats) ||
		  king_in_check_rightward(board, player, king, threats));
}

static int king_in_check_northwest (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	if (king & FC_LEFT_COL) {
		return 0;
	}

	for (uint64_t i = king << 7; i; i <<= 7) {
		if (i & threats) {
			return 1;
		}
		if (i & FC_ALL_ALLIES((*board), player)) {
			break;
		}
		if (i & FC_LEFT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_southwest (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	if (king & FC_LEFT_COL) {
		return 0;
	}

	for (uint64_t i = king >> 9; i; i >>= 9) {
		if (i & threats) {
			return 1;
		}
		if (i & FC_ALL_ALLIES((*board), player)) {
			break;
		}
		if (i & FC_LEFT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_northeast (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	if (king & FC_RIGHT_COL) {
		return 0;
	}

	for (uint64_t i = king << 9; i; i <<= 9) {
		if (i & threats) {
			return 1;
		}
		if (i & FC_ALL_ALLIES((*board), player)) {
			break;
		}
		if (i & FC_RIGHT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_southeast (fc_board_t *board, fc_player_t player,
		uint64_t king, uint64_t threats)
{
	if (king & FC_RIGHT_COL) {
		return 0;
	}

	for (uint64_t i = king >> 7; i; i >>= 7) {
		if (i & threats) {
			return 1;
		}
		if (i & FC_ALL_ALLIES((*board), player)) {
			break;
		}
		if (i & FC_RIGHT_COL) {
			break;
		}
	}
	return 0;
}

static int king_in_check_diagonally (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t threats = FC_BITBOARD((*board), FC_NEXT_PLAYER(player),
			FC_QUEEN);
	threats |= FC_BITBOARD((*board), FC_NEXT_PLAYER(player), FC_BISHOP);
	threats |= FC_BITBOARD((*board), FC_PARTNER(FC_NEXT_PLAYER(player)),
			FC_QUEEN);
	threats |= FC_BITBOARD((*board), FC_PARTNER(FC_NEXT_PLAYER(player)),
			FC_BISHOP);

	return (king_in_check_northwest(board, player, king, threats) ||
		king_in_check_southwest(board, player, king, threats) ||
		king_in_check_northeast(board, player, king, threats) ||
		king_in_check_southeast(board, player, king, threats));
}

static int king_in_check_by_knight (fc_board_t *board, fc_player_t player,
		uint64_t king)
{
	uint64_t knights;
	knights = FC_BITBOARD((*board), FC_NEXT_PLAYER(player), FC_KNIGHT) |
		  FC_BITBOARD((*board), FC_PARTNER(FC_NEXT_PLAYER(player)),
					  FC_KNIGHT);
	if (!(king & (FC_LEFT_COL | FC_2LEFT_COL))) {
		if (((king << 6) & knights) || ((king >> 10) & knights)) {
			return 1;
		}
	}
	if (!(king & FC_LEFT_COL)) {
		if (((king << 15) & knights) || ((king >> 17) & knights)) {
			return 1;
		}
	}
	if (!(king & (FC_RIGHT_COL | FC_2RIGHT_COL))) {
		if (((king << 10) & knights) || ((king >> 6) & knights)) {
			return 1;
		}
	}
	if (!(king & FC_RIGHT_COL)) {
		if (((king << 17) & knights) || ((king >> 15) & knights)) {
			return 1;
		}
	}
	return 0;
}

/*
 * Returns FC_CHECK (1) if king is in check; 0 otherwise.
 */
static int is_check (fc_board_t *board, fc_player_t player)
{
	uint64_t king = FC_BITBOARD((*board), player, FC_KING);
	return (king_in_check_laterally(board, player, king) ||
		king_in_check_diagonally(board, player, king) ||
		is_threatened_by_pawn(board, player, king) ||
		is_threatened_by_king(board, player, king) ||
		king_in_check_by_knight(board, player, king));
}

/*
 * Returns FC_CHECK if player's king is in check, FC_CHECKMATE if checkmate,
 * and 0 otherwise.
 */
/* FIXME TODO
 * 1. rename this function to something more in line with the rest of the API
 *    so far, e.g. fc_board_check_status().
 * 2. Move all the check code to it's own source file, but keep the board.h
 *    header the same.
 */
int fc_is_king_in_check (fc_board_t *board, fc_player_t player)
{
	if (!is_check(board, player)) {
		return 0;
	}

	fc_mlist_t moves;
	fc_mlist_init(&moves, 0);
	fc_board_get_moves(board, &moves, player);
	for (int i = 0; i < fc_mlist_length(&moves); i++) {
		fc_board_t copy;
		fc_board_copy(&copy, board);
		fc_board_make_move(&copy, fc_mlist_get(&moves, i));
		if (!is_check(&copy, player)) {
			fc_mlist_free(&moves);
			return FC_CHECK;
		}
	}

	fc_mlist_free(&moves);
	return FC_CHECKMATE;
}
