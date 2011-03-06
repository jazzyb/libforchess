#include <stdio.h>
#include <stdlib.h>

#include "forchess/board.h"

#define FC_BITBOARD(board, player, piece) (board[player * 6 + piece])

int fc_set_piece (fc_board_t *board,
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
	return 1;
}

int fc_get_piece (fc_board_t *board,
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
 * The format of a config file is:
 *     1 N c3
 *     3 K h8
 * etc.
 *
 * TODO make this function prettier and more robust.  For example, currently
 * two different pieces are allowed to occupy the same space; we should
 * probably fail if that is the case.  Also think about adding errno.
 */
int fc_setup_board (fc_board_t *board, const char *filename)
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

		fc_set_piece(board, player, p, row - '1', col - 'a');
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

/*
 * Cycles through each piece (bit) on the bitboard.
 * piece is the bit
 * x is a bitboard
 */
#define FC_FOREACH(piece, x) \
	for(piece = (x & (~x + 1)); x; x ^= piece, piece = (x & (~x + 1)))

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
		switch (player) {
		case FC_FIRST:
			pawn_move_if_valid(board, moves, FC_FIRST, pawn,
					   pawn << 9, pawn << 8, pawn << 1);
			break;
		case FC_SECOND:
			pawn_move_if_valid(board, moves, FC_SECOND, pawn,
					   pawn >> 7, pawn >> 8, pawn << 1);
			break;
		case FC_THIRD:
			pawn_move_if_valid(board, moves, FC_THIRD, pawn,
					   pawn >> 9, pawn >> 8, pawn >> 1);
			break;
		case FC_FOURTH:
			pawn_move_if_valid(board, moves, FC_FOURTH, pawn,
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

void fc_get_moves (fc_board_t *board, fc_mlist_t *moves, fc_player_t player)
{
	fc_get_king_moves(board, moves, player);
	fc_get_knight_moves(board, moves, player);
	fc_get_pawn_moves(board, moves, player);
	fc_get_bishop_moves(board, moves, player);
	fc_get_rook_moves(board, moves, player);
}
