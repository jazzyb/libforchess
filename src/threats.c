#include <assert.h>

#include "forchess/board.h"

#define FC_THREAT_BB(board, player, piece) \
	(*board)[FC_START_THREATS + (6 * player + piece)]

static void update_pawn_threats (fc_board_t *board, fc_player_t player)
{
	uint64_t pawn, pawns = FC_BITBOARD((*board), player, FC_PAWN);
	uint64_t newbb = pawns;
	FC_FOREACH(pawn, pawns) {
		switch (fc_get_pawn_orientation(board, pawn)) {
		case FC_FIRST:
			newbb |= pawn << 1;
			newbb |= pawn << 8;
			break;
		case FC_SECOND:
			newbb |= pawn << 1;
			newbb |= pawn >> 8;
			break;
		case FC_THIRD:
			newbb |= pawn >> 1;
			newbb |= pawn >> 8;
			break;
		case FC_FOURTH:
			newbb |= pawn >> 1;
			newbb |= pawn << 8;
			break;
		}
	}
	FC_THREAT_BB(board, player, FC_PAWN) = newbb;
}

static void update_king_threats (fc_board_t *board, fc_player_t player)
{
	uint64_t king = FC_BITBOARD((*board), player, FC_KING);
	uint64_t newbb = king;
	if (!(king & FC_LEFT_COL)) {
		newbb |= king << 7;
		newbb |= king >> 1;
		newbb |= king >> 9;
	}
	newbb |= king << 8;
	newbb |= king >> 8;
	if (!(king & FC_RIGHT_COL)) {
		newbb |= king << 9;
		newbb |= king << 1;
		newbb |= king >> 7;
	}
	FC_THREAT_BB(board, player, FC_KING) = newbb;
}

static void update_knight_threats (fc_board_t *board, fc_player_t player)
{
	uint64_t knight, knights = FC_BITBOARD((*board), player, FC_KNIGHT);
	uint64_t newbb = knights;
	FC_FOREACH(knight, knights) {
		if (!(knight & (FC_LEFT_COL | FC_2LEFT_COL))) {
			newbb |= knight << 6;
			newbb |= knight >> 10;
		}
		if (!(knight & FC_LEFT_COL)) {
			newbb |= knight << 15;
			newbb |= knight >> 17;
		}
		if (!(knight & (FC_RIGHT_COL | FC_2RIGHT_COL))) {
			newbb |= knight << 10;
			newbb |= knight >> 6;
		}
		if (!(knight & FC_RIGHT_COL)) {
			newbb |= knight << 17;
			newbb |= knight >> 15;
		}
	}
	FC_THREAT_BB(board, player, FC_KNIGHT) = newbb;
}

static uint64_t northwest_threats (fc_board_t *board, fc_player_t player,
		uint64_t bit)
{
	if (bit & FC_LEFT_COL) {
		return UINT64_C(0);
	}

	uint64_t ret = UINT64_C(0);
	for (uint64_t i = bit << 7; i; i <<= 7) {
		ret |= i;
		if ((i & FC_LEFT_COL) || !fc_is_empty(board, i)) {
			break;
		}
	}
	return ret;
}

static uint64_t southwest_threats (fc_board_t *board, fc_player_t player,
		uint64_t bit)
{
	if (bit & FC_LEFT_COL) {
		return UINT64_C(0);
	}

	uint64_t ret = UINT64_C(0);
	for (uint64_t i = bit >> 9; i; i >>= 9) {
		ret |= i;
		if ((i & FC_LEFT_COL) || !fc_is_empty(board, i)) {
			break;
		}
	}
	return ret;
}

static uint64_t northeast_threats (fc_board_t *board, fc_player_t player,
		uint64_t bit)
{
	if (bit & FC_RIGHT_COL) {
		return UINT64_C(0);
	}

	uint64_t ret = UINT64_C(0);
	for (uint64_t i = bit << 9; i; i <<= 9) {
		ret |= i;
		if ((i & FC_RIGHT_COL) || !fc_is_empty(board, i)) {
			break;
		}
	}
	return ret;
}

static uint64_t southeast_threats (fc_board_t *board, fc_player_t player,
		uint64_t bit)
{
	if (bit & FC_RIGHT_COL) {
		return UINT64_C(0);
	}

	uint64_t ret = UINT64_C(0);
	for (uint64_t i = bit >> 7; i; i >>= 7) {
		ret |= i;
		if ((i & FC_RIGHT_COL) || !fc_is_empty(board, i)) {
			break;
		}
	}
	return ret;
}

static void update_bishop_threats (fc_board_t *board, fc_player_t player)
{
	uint64_t bishop, bishops = FC_BITBOARD((*board), player, FC_BISHOP);
	uint64_t newbb = bishops;
	FC_FOREACH(bishop, bishops) {
		newbb |= northwest_threats(board, player, bishop);
		newbb |= southwest_threats(board, player, bishop);
		newbb |= northeast_threats(board, player, bishop);
		newbb |= southeast_threats(board, player, bishop);
	}
	FC_THREAT_BB(board, player, FC_BISHOP) = newbb;
}

static uint64_t upward_threats (fc_board_t *board, fc_player_t player,
		uint64_t bit)
{
	uint64_t ret = UINT64_C(0);
	for (uint64_t i = bit << 8; i; i <<= 8) {
		ret |= i;
		if (!fc_is_empty(board, i)) {
			break;
		}
	}
	return ret;
}

static uint64_t downward_threats (fc_board_t *board, fc_player_t player,
		uint64_t bit)
{
	uint64_t ret = UINT64_C(0);
	for (uint64_t i = bit >> 8; i; i >>= 8) {
		ret |= i;
		if (!fc_is_empty(board, i)) {
			break;
		}
	}
	return ret;
}

static uint64_t leftward_threats (fc_board_t *board, fc_player_t player,
		uint64_t bit)
{
	if (bit & FC_LEFT_COL) {
		return UINT64_C(0);
	}

	uint64_t ret = UINT64_C(0);
	for (uint64_t i = bit >> 1; i; i >>= 1) {
		ret |= i;
		if ((i & FC_LEFT_COL) || !fc_is_empty(board, i)) {
			break;
		}
	}
	return ret;
}

static uint64_t rightward_threats (fc_board_t *board, fc_player_t player,
		uint64_t bit)
{
	if (bit & FC_RIGHT_COL) {
		return UINT64_C(0);
	}

	uint64_t ret = UINT64_C(0);
	for (uint64_t i = bit << 1; i; i <<= 1) {
		ret |= i;
		if ((i & FC_RIGHT_COL) || !fc_is_empty(board, i)) {
			break;
		}
	}
	return ret;
}

static void update_rook_threats (fc_board_t *board, fc_player_t player)
{
	uint64_t rook, rooks = FC_BITBOARD((*board), player, FC_ROOK);
	uint64_t newbb = rooks;
	FC_FOREACH(rook, rooks) {
		newbb |= upward_threats(board, player, rook);
		newbb |= downward_threats(board, player, rook);
		newbb |= leftward_threats(board, player, rook);
		newbb |= rightward_threats(board, player, rook);
	}
	FC_THREAT_BB(board, player, FC_ROOK) = newbb;
}

static void update_queen_threats (fc_board_t *board, fc_player_t player)
{
	uint64_t queen, queens = FC_BITBOARD((*board), player, FC_QUEEN);
	uint64_t newbb = queens;
	FC_FOREACH(queen, queens) {
		newbb |= northwest_threats(board, player, queen);
		newbb |= southwest_threats(board, player, queen);
		newbb |= northeast_threats(board, player, queen);
		newbb |= southeast_threats(board, player, queen);
		newbb |= upward_threats(board, player, queen);
		newbb |= downward_threats(board, player, queen);
		newbb |= leftward_threats(board, player, queen);
		newbb |= rightward_threats(board, player, queen);
	}
	FC_THREAT_BB(board, player, FC_QUEEN) = newbb;
}

static void update_threat_bitboard (fc_board_t *board, fc_player_t player,
		fc_piece_t piece)
{
	switch (piece) {
	case FC_PAWN:
		update_pawn_threats(board, player);
		break;
	case FC_BISHOP:
		update_bishop_threats(board, player);
		break;
	case FC_KNIGHT:
		update_knight_threats(board, player);
		break;
	case FC_ROOK:
		update_rook_threats(board, player);
		break;
	case FC_QUEEN:
		update_queen_threats(board, player);
		break;
	case FC_KING:
		update_king_threats(board, player);
		break;
	default:
		assert(0);
	}
}

void fc_update_threats_from_move (fc_board_t *board, fc_move_t *move)
{
	(*board)[FC_TEAM1_THREATS] = (*board)[FC_TEAM2_THREATS] = UINT64_C(0);
	for (int i = FC_START_THREATS; i <= FC_END_THREATS; i++) {
		fc_player_t player = (i - FC_START_THREATS) / 6;
		fc_piece_t piece = (i - FC_START_THREATS) % 6;
		if ((*board)[i] & move->move) {
			update_threat_bitboard(board, player, piece);
		}
		(*board)[FC_TEAM1_THREATS + (player % 2)] |= (*board)[i];
	}
}

void fc_update_all_threats (fc_board_t *board)
{
	(*board)[FC_TEAM1_THREATS] = (*board)[FC_TEAM2_THREATS] = UINT64_C(0);
	for (int i = FC_START_THREATS; i <= FC_END_THREATS; i++) {
		fc_player_t player = (i - FC_START_THREATS) / 6;
		fc_piece_t piece = (i - FC_START_THREATS) % 6;
		update_threat_bitboard(board, player, piece);
		(*board)[FC_TEAM1_THREATS + (player % 2)] |= (*board)[i];
	}
}
