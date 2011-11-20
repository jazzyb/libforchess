#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "forchess/board.h"


/*
 * Mark all positions on the board that are empty.
 */
static void update_empty_positions (fc_board_t *b)
{
	b->bitb[FC_EMPTY_SPACES] = ~(FC_ALL_PIECES(b, 0) |
			FC_ALL_PIECES(b, 1) | FC_ALL_PIECES(b, 2) |
			FC_ALL_PIECES(b, 3));
}

static void set_material_values_to_defaults (fc_board_t *board)
{
	fc_piece_t i;
	int _default_piece_value[FC_NUM_PIECES] = {
		100,	/* pawns */
		300,	/* bishops */
		350,	/* knights */
		500,	/* rooks */
		900,	/* queens */
		100000	/* kings */
	};

	for (i = FC_PAWN; i <= FC_KING; i++) {
		board->piece_value[i] = _default_piece_value[i];
	}
}

/*
 * Initialize the bitboard values.
 */
void fc_board_init (fc_board_t *board)
{
	bzero(board->bitb, sizeof(fc_board_t));
	update_empty_positions(board);
	set_material_values_to_defaults(board);
}

/* FIXME switch the row/col values to x/y ones; the current way seems backwards
 * */
int fc_board_set_piece (fc_board_t *board, fc_player_t player, fc_piece_t piece,
			int row, int col)
{
	uint64_t bb;

	assert(board);
	/*
	 * FIXME make sure there is not already a piece on this position.
	 * FIXME also make sure we don't call it with bad row/col values.
	 */
	bb = UINT64_C(1) << (row * 8 + col);
	FC_BITBOARD(board, player, piece) |= bb;
	if (piece == FC_PAWN) {
		FC_PAWN_BB(board, player) |= bb;
	}
	update_empty_positions(board);
	return 1;
}

/*
 * Search through the bit boards and find the player and piece associated with
 * the given bit.  The function assumes that the value bit has only one bit
 * turned on.
 */
static void find_player_piece (fc_board_t *board, fc_player_t *player,
		fc_piece_t *piece, uint64_t bit)
{
	int i;

	if (board->bitb[FC_EMPTY_SPACES] & bit) {
		*player = *piece = FC_NONE;
		return;
	}

	/* NOTE: I'm using 24 below instead of FC_TOTAL_BITBOARDS because I
	 * only want to look at the bitboards that represent pieces */
	for (i = 0; i < 24; i++) {
		if (board->bitb[i] & bit) {
			*player = i / 6;
			*piece = i % 6;
			return;
		}
	}

	/* We should never reach this point. */
	*player = *piece = FC_NONE;
	assert(0);
}

/*
 * Returns 1 and sets the values of player and piece if it found a piece at the
 * given row and col.  Returns 0 if the space was empty.
 */
int fc_board_get_piece (fc_board_t *board, fc_player_t *player,
		fc_piece_t *piece, int row, int col)
{
	uint64_t bit;

	assert(board && player && piece);

	bit = UINT64_C(1) << (row * 8 + col);
	find_player_piece(board, player, piece, bit);
	return (*player != FC_NONE && *piece != FC_NONE);
}

/*
 * Removes any piece at the given row and column.
 * Returns 0 if there was no piece at the given coordinates; 1 otherwise.
 */
int fc_board_remove_piece (fc_board_t *board, int row, int col)
{
	int i;
	uint64_t bit;
	fc_player_t player;
	fc_piece_t piece;

	assert(board);

	fc_board_get_piece(board, &player, &piece, row, col);
	bit = UINT64_C(1) << (row * 8 + col);
	for (i = 0; i < 24; i++) {
		if (board->bitb[i] & bit) {
			if (piece == FC_PAWN) {
				FC_PAWN_BB(board, player) ^= bit;
			}
			board->bitb[i] ^= bit;
			update_empty_positions(board);
			return 1;
		}
	}
	return 0;
}

void fc_board_set_material_value (fc_board_t *board, fc_piece_t piece,
		int value)
{
	assert(board);
	board->piece_value[piece] = value;
}

int fc_board_get_material_value (fc_board_t *board, fc_piece_t piece)
{
	assert(board);
	return board->piece_value[piece];
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
int fc_board_setup (fc_board_t *board, const char *filename, fc_player_t *first)
{
	FILE *fp;
	int read, player;
	char piece, col, row;
	fc_piece_t p;

	assert(board && filename && first);

	fp = fopen(filename, "r");
	if (!fp) {
		return 0;
	}

	*first = FC_NONE;

	read = fscanf(fp, "%d %c %c%c \n", &player, &piece, &col, &row);
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
		if (*first == FC_NONE) {
			*first = player;
		}

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
static int may_move_to (fc_board_t *b, fc_player_t p, uint64_t m)
{
	return !(m & FC_ALL_ALLIES(b, p));
}

/*
 * This function updates the move list with the new move if the piece in
 * question is allowed to move to the given space.
 *
 * NOTE: This function is only used for king and knight; the corresponding
 * function for pawns is called pawn_move_if_valid() and move_and_continue()
 * for bishop, rook, and queen.
 */
static void move_if_valid (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece,
		uint64_t space)
{
	fc_move_t move;
	fc_player_t *opp_player = &(move.opp_player);
	fc_piece_t *opp_piece = &(move.opp_piece);

	move.player = player;
	move.piece = type;
	move.promote = FC_NONE;
	move.move = piece | space;

	if (space && may_move_to(board, player, space)) {
		find_player_piece(board, opp_player, opp_piece, space);
		fc_mlist_append(moves, &move);
	}
}

/* assuming there is only one king per player */
void fc_get_king_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player)
{
	uint64_t king;

	king = FC_BITBOARD(board, player, FC_KING);
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

void fc_get_knight_moves (fc_board_t *board, fc_mlist_t *moves,
		 fc_player_t player)
{
	uint64_t knight, bb;

	bb = FC_BITBOARD(board, player, FC_KNIGHT);
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
static int is_occupied_by_enemy (fc_board_t *b, fc_player_t p,
					uint64_t m)
{
	return !!(m & FC_ALL_ALLIES(b, ((p + 1) % 4)));
}

int fc_is_empty (fc_board_t *b, uint64_t m)
{
	return !!(m & b->bitb[FC_EMPTY_SPACES]);
}

/*
 * Basically the same as move_if_valid() above with the following changes:
 * 	m1 represents the single available pawn diagonal movement
 * 	m2 and m3 are the lateral capture moves
 */
static void pawn_move_if_valid (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, uint64_t pawn, uint64_t m1, uint64_t m2,
		uint64_t m3)
{
	fc_move_t move;
	fc_player_t *opp_player = &(move.opp_player);
	fc_piece_t *opp_piece = &(move.opp_piece);

	move.player = player;
	move.piece = FC_PAWN;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;

	if (fc_is_empty(board, m1)) {
		move.move = pawn | m1;
		fc_mlist_append(moves, &move);
	}

	if (is_occupied_by_enemy(board, player, m2)) {
		move.move = pawn | m2;
		find_player_piece(board, opp_player, opp_piece, m2);
		fc_mlist_append(moves, &move);
	}
	if (is_occupied_by_enemy(board, player, m3)) {
		move.move = pawn | m3;
		find_player_piece(board, opp_player, opp_piece, m3);
		fc_mlist_append(moves, &move);
	}
}

fc_player_t fc_get_pawn_orientation (fc_board_t *board, uint64_t pawn)
{
	if (pawn & board->bitb[FC_FIRST_PAWNS]) {
		return FC_FIRST;
	} else if (pawn & board->bitb[FC_SECOND_PAWNS]) {
		return FC_SECOND;
	} else if (pawn & board->bitb[FC_THIRD_PAWNS]) {
		return FC_THIRD;
	} else if (pawn & board->bitb[FC_FOURTH_PAWNS]) {
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
void fc_get_pawn_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player)
{
	uint64_t pawn, bb;

	bb = FC_BITBOARD(board, player, FC_PAWN);
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
		default:
			assert(0);
		}
	}
}

/*
 * This function appends a move to the mlist if piece is allowed to move to
 * space.  Returns 1 if the calling function may continue evaluating moves
 * (i.e. the space is empty); 0 otherwise.
 */
static int move_and_continue (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece,
		uint64_t space)
{
	fc_move_t move;
	fc_player_t *opp_player = &(move.opp_player);
	fc_piece_t *opp_piece = &(move.opp_piece);

	move.player = player;
	move.piece = type;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = piece | space;

	if (fc_is_empty(board, space)) {
		fc_mlist_append(moves, &move);
		return 1;
	} else if (is_occupied_by_enemy(board, player, space)) {
		find_player_piece(board, opp_player, opp_piece, space);
		fc_mlist_append(moves, &move);
	}
	return 0;
}

static void fc_get_northwest_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece)
{
	uint64_t i;

	if (piece & FC_LEFT_COL) {
		return;
	}

	for (i = piece << 7; i; i <<= 7) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_LEFT_COL) {
			break;
		}
	}
}

static void fc_get_southwest_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece)
{
	uint64_t i;

	if (piece & FC_LEFT_COL) {
		return;
	}

	for (i = piece >> 9; i; i >>= 9) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_LEFT_COL) {
			break;
		}
	}
}

static void fc_get_northeast_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece)
{
	uint64_t i;

	if (piece & FC_RIGHT_COL) {
		return;
	}

	for (i = piece << 9; i; i <<= 9) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_RIGHT_COL) {
			break;
		}
	}
}

static void fc_get_southeast_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece)
{
	uint64_t i;

	if (piece & FC_RIGHT_COL) {
		return;
	}

	for (i = piece >> 7; i; i >>= 7) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_RIGHT_COL) {
			break;
		}
	}
}

void fc_get_bishop_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player)
{
	uint64_t bishop, bb;

	bb = FC_BITBOARD(board, player, FC_BISHOP);
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

static void fc_get_upward_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece)
{
	uint64_t i;

	for (i = piece << 8; i; i <<= 8) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}
	}
}

static void fc_get_downward_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece)
{
	uint64_t i;

	for (i = piece >> 8; i; i >>= 8) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}
	}
}

static void fc_get_leftward_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece)
{
	uint64_t i;

	if (piece & FC_LEFT_COL) {
		return;
	}

	for (i = piece >> 1; i; i >>= 1) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_LEFT_COL) {
			break;
		}
	}
}

static void fc_get_rightward_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player, fc_piece_t type, uint64_t piece)
{
	uint64_t i;

	if (piece & FC_RIGHT_COL) {
		return;
	}

	for (i = piece << 1; i; i <<= 1) {
		if (!move_and_continue(board, moves, player, type, piece, i)) {
			break;
		}

		if (i & FC_RIGHT_COL) {
			break;
		}
	}
}

void fc_get_rook_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player)
{
	uint64_t rook, bb;

	bb = FC_BITBOARD(board, player, FC_ROOK);
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

void fc_get_queen_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player)
{
	uint64_t queen, bb;

	bb = FC_BITBOARD(board, player, FC_QUEEN);
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

void fc_board_get_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player)
{
	assert(board && moves);
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
void fc_board_get_removes (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player)
{
	uint64_t piece, bb;
	fc_move_t move;

	move.player = player;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;

	assert(board && moves);
	for (move.piece = FC_PAWN; move.piece <= FC_KING; move.piece++) {
		bb = FC_BITBOARD(board, player, move.piece);
		FC_FOREACH(piece, bb) {
			move.move = piece;
			fc_mlist_append(moves, &move);
		}
	}
}

/*
 * Give all player 'from's pieces to player 'to'.
 */
static void fc_convert_pieces (fc_board_t *board, fc_player_t from,
		fc_player_t to)
{
	uint64_t pawns;
	fc_bitboards_t i;
	fc_piece_t j;

	pawns = FC_BITBOARD(board, from, FC_PAWN);
	for (i = FC_FIRST_PAWNS; i <= FC_FOURTH_PAWNS; i++) {
		if (pawns & board->bitb[i]) {
			FC_BITBOARD(board, to, FC_PAWN) |= board->bitb[i];
		}
	}
	FC_BITBOARD(board, from, FC_PAWN) = UINT64_C(0);

	for (j = FC_PAWN + 1; j < FC_KING; j++) {
		FC_BITBOARD(board, to, j) |= FC_BITBOARD(board, from, j);
		FC_BITBOARD(board, from, j) = UINT64_C(0);
	}
}

/*
 * If the given pawn reaches its "back wall" it will get promoted.
 */
#define FC_FIRST_BACK_WALL  (UINT64_C(0xff80808080808080))
#define FC_SECOND_BACK_WALL (UINT64_C(0x80808080808080ff))
#define FC_THIRD_BACK_WALL  (UINT64_C(0x01010101010101ff))
#define FC_FOURTH_BACK_WALL (UINT64_C(0xff01010101010101))
static int must_promote (fc_player_t player, uint64_t pawn)
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
	return -1; /* never reaches here */
}

/*
 * Find the piece bitboard that needs to be updated for the enemy of player.
 */
static void update_enemy_bitboards (fc_board_t *board, fc_move_t *move,
		fc_player_t side, uint64_t bit)
{
	if (move->opp_player == FC_NONE && move->opp_piece == FC_NONE) {
		return;
	}
	assert(bit);

	FC_BITBOARD(board, move->opp_player, move->opp_piece) ^= bit;
	if (move->opp_piece == FC_PAWN) {
		assert(side != FC_NONE);
		FC_PAWN_BB(board, side) ^= bit;
	} else if (move->opp_piece == FC_KING) {
		fc_convert_pieces(board, move->opp_player, move->player);
	}
}

/*
 * Returns 1 if the move requires a pawn to be promoted; 0 otherwise.
 */
int fc_board_move_requires_promotion (fc_board_t *board, fc_move_t *move,
		fc_player_t *side)
{
	uint64_t pawn;

	if (move->piece != FC_PAWN) {
		return 0;
	}
	pawn = (FC_BITBOARD(board, move->player, FC_PAWN) ^ move->move) &
		move->move;
	*side = fc_get_pawn_orientation(board, pawn ^ move->move);
	return must_promote(*side, pawn);
}

/*
 * Update the board with the given move.  Returns 0 iff there is no piece
 * specified in the case of a pawn promotion; returns 1 otherwise.
 */
int fc_board_make_move (fc_board_t *board, fc_move_t *move)
{
	uint64_t b;
	/* side is the orientation of the pawn (if the move is a pawn) */
	fc_player_t side, enemy_side;

	assert(board && move);

	if (fc_board_move_requires_promotion(board, move, &side)) {
		if (move->promote == FC_NONE) {
			return 0;
		} else {
			return fc_board_make_pawn_move(board, move,
					move->promote);
		}
	}

	/*
	 * Move the player's piece; then get the second bit (b) that represents
	 * the possible captured piece.
	 */
	FC_BITBOARD(board, move->player, move->piece) ^= move->move;
	b = FC_BITBOARD(board, move->player, move->piece) & move->move;

	/*
	 * NOTE: We are getting the orientation for the enemy pawn here because
	 * if we wait until after we have updated the below pawn orientation
	 * bitboard, then there will be two bitboards which share a bit, and we
	 * won't know which one it belongs to.
	 */
	enemy_side = fc_get_pawn_orientation(board, b);

	if (move->piece == FC_PAWN) {
		side = fc_get_pawn_orientation(board, b ^ move->move);
		FC_PAWN_BB(board, side) ^= move->move;
	}

	update_enemy_bitboards(board, move, enemy_side, b);
	update_empty_positions(board);

	return 1;
}

/*
 * If a pawn is moved in such a way that it must be promoted and the move
 * struct does not have a valid piece to promote to, then fc_board_make_move()
 * will return 0.  In that case, the user must call the below function with a
 * third argument declaring what piece the pawn should be promoted to.
 */
int fc_board_make_pawn_move (fc_board_t *board, fc_move_t *move,
			     fc_piece_t new_piece)
{
	uint64_t pawn;
	fc_player_t orientation;
	fc_move_t copy;

	assert(board && move);

	if (move->piece != FC_PAWN) {
		return 0;
	}
	pawn = FC_BITBOARD(board, move->player, FC_PAWN) & move->move;

	switch(new_piece) {
	case FC_BISHOP:
	case FC_KNIGHT:
	case FC_ROOK:
	case FC_QUEEN:
		FC_BITBOARD(board, move->player, new_piece) |= pawn;
		break;
	default:
		return 0;
	}
	FC_BITBOARD(board, move->player, FC_PAWN) ^= pawn;
	orientation = fc_get_pawn_orientation(board, pawn);
	FC_PAWN_BB(board, orientation) ^= pawn;

	fc_move_copy(&copy, move);
	copy.piece = new_piece;
	return fc_board_make_move(board, &copy);
}

void fc_board_copy (fc_board_t *dst, fc_board_t *src)
{
	int i;
	fc_piece_t p;

	assert(dst && src);

	for (i = 0; i < FC_TOTAL_BITBOARDS; i++) {
		dst->bitb[i] = src->bitb[i];
	}
	for (p = FC_PAWN; p <= FC_KING; p++) {
		dst->piece_value[p] = src->piece_value[p];
	}
}

