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

#ifndef _FC_BOARD_H_
#define _FC_BOARD_H_

/**
 * @file forchess/board.h
 * @author jazzyb aka Jason M Barnes
 * @brief API for the LibForchess board manipulation code.
 */

#ifndef DOXYGEN_IGNORE

#include <stdint.h>
#include <stdio.h>

#include "forchess/moves.h"

typedef enum {
	/*
	 * Why start with 24?  Because we have to keep track of 6 different
	 * pieces for 4 players before this "first" bitboard.
	 */
	FC_FIRST_PAWNS = 24,
	FC_SECOND_PAWNS,
	FC_THIRD_PAWNS,
	FC_FOURTH_PAWNS,

	/* Keep track of empty positions on the board. */
	FC_EMPTY_SPACES,

	/* This must always be last. */
	FC_TOTAL_BITBOARDS
} fc_bitboards_t;

typedef struct {
	uint64_t bitb[FC_TOTAL_BITBOARDS];
	int piece_value[FC_NUM_PIECES];
} fc_board_t;

/* The following are used with the mlist_iter_t struct in the AI code.  I'm
 * not sure yet what to do with these functions or how the API ought to be
 * used.  Possible FIXME.
 */
typedef struct {
	fc_board_t *board;
	fc_player_t player;
	int current_check_status;
	int partner_check_status;
	int all_moves_are_invalid;
	int initial_flag;
} fc_board_state_t;
void fc_board_state_init (fc_board_state_t *state, fc_board_t *board,
		fc_player_t player);
/* mlist_iter callback */
fc_move_t *fc_board_get_next_move (fc_mlist_iter_t *iter);

/* macro to get the first 24 bitboards representing pieces */
#define FC_BITBOARD(board, player, piece) (board->bitb[player * 6 + piece])

/*
 * Cycles through each piece (bit) on the bitboard.
 * piece is the bit
 * x is a bitboard
 */
#define FC_FOREACH(piece, x) \
	for(piece = (x & (~x + 1)); x; x ^= piece, piece = (x & (~x + 1)))

/* macro to get a particular pawn orientation bitboard */
#define FC_PAWN_BB(board, orientation) \
	(board->bitb[FC_FIRST_PAWNS + orientation])

/*
 * NOTE:  The below macros represent the following values:
 *
 * #define FC_LEFT_COL  (UINT64_C(0x0101010101010101))
 * #define FC_RIGHT_COL (UINT64_C(0x8080808080808080))
 * #define FC_2LEFT_COL  (UINT64_C(0x0202020202020202))
 * #define FC_2RIGHT_COL (UINT64_C(0x4040404040404040))
 *
 * References to UINT64_C had to be removed as it won't build on a 32-bit
 * system.
 */
/* used to check a piece's position on the board */
#define FC_LEFT_COL ((((uint64_t)0x01010101) << 32) | ((uint64_t)0x01010101))
#define FC_RIGHT_COL ((((uint64_t)0x80808080) << 32) | ((uint64_t)0x80808080))
#define FC_2LEFT_COL ((((uint64_t)0x02020202) << 32) | ((uint64_t)0x02020202))
#define FC_2RIGHT_COL ((((uint64_t)0x40404040) << 32) | ((uint64_t)0x40404040))

/* returns a bitboard where all pieces for a player are represented by 1 */
#define FC_ALL_PIECES(b, p) \
	(b->bitb[6*p] | b->bitb[6*p+1] | b->bitb[6*p+2] | \
	 b->bitb[6*p+3] | b->bitb[6*p+4] | b->bitb[6*p+5])

/* returns all pieces for a pair of players */
#define FC_ALL_ALLIES(b, p) \
	(FC_ALL_PIECES(b, p) | FC_ALL_PIECES(b, ((p + 2) % 4)))

/*
 * Don't know if this is the best place to put this function.  It's not
 * *really* a part of the API, but it's used by functions in both board.c and
 * check.c.
 */
int fc_is_empty (fc_board_t *board, uint64_t bit);
fc_player_t fc_get_pawn_orientation (fc_board_t *board, uint64_t pawn);

#endif /* DOXYGEN_IGNORE */

/**
 * @brief Initializes the values for an empty board.
 *
 * Sets all the values on the bitboard to zero and sets the FC_EMPTY_SPACES
 * board to all F.
 *
 * @param[in,out] board A pointer to the game board.
 *
 * @return void
 */
void fc_board_init (fc_board_t *board);

/**
 * @brief Initializes the board based on a config file.
 *
 * The forchess configuration file is one where each line is of the form:
 * 	<number> <piece> <coordinates>
 * where <number> represents the player (e.g. 1 for the first player, 2 for the
 * second player), <piece> represents one of the common character codes for a
 * chess piece (i.e. P, B, N, R, Q, K), and <coordinates> represents a chess
 * coordinate (e.g. "a6" or "f2").  No blank lines are allowed.  The first
 * player that is encountered in the config file will be returned in the
 * 'first' parameter.
 *
 * @param[out] board A pointer to the game board to be initialized; the board
 * should be allocated before the call to this function.
 * @param[in] filename The name of the config file.
 * @param[out] first The first player in config file.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_board_setup (fc_board_t *board, const char *filename,
		fc_player_t *first);

/**
 * @brief Places piece on the board at row and column.
 *
 * @param[in,out] board A pointer to the game board.
 * @param[in] player The player.
 * @param[in] piece The piece.
 * @param[in] row The rows of the chess board start at 0 (the one commonly
 * labeled as "1") and go to 7 ("8").
 * @param[in] col The columns of the chess board start at 0 (the one commonly
 * labeled as "a") and go to 7 ("h").
 *
 * @return 1 on success; 0 otherwise
 */
int fc_board_set_piece (fc_board_t *board, fc_player_t player, fc_piece_t piece,
			int row, int col);

/**
 * @brief Returns the player's piece that resides at the row and column on the
 * board.
 *
 * The function sets the values of player and piece for the piece at row/col.
 * If no piece resides at those coordinates on the board, then player and piece
 * are untouched, and 0 is returned.
 *
 * @param[in] board A pointer to the game board.
 * @param[out] player The player to be set.
 * @param[out] piece The piece to be set.
 * @param[in] row The rows of the chess board start at 0 (the one commonly
 * labeled as "1") and go to 7 ("8").
 * @param[in] col The columns of the chess board start at 0 (the one commonly
 * labeled as "a") and go to 7 ("h").
 *
 * @return 1 if player and piece were set successfully; 0 otherwise
 */
int fc_board_get_piece (fc_board_t *board, fc_player_t *player,
		  fc_piece_t *piece, int row, int col);

/**
 * @brief Removes a piece from the board.
 *
 * @param[out] board A pointer to the game board.
 * @param[in] row The rows of the chess board start at 0 (the one commonly
 * labeled as "1") and go to 7 ("8").
 * @param[in] col The columns of the chess board start at 0 (the one commonly
 * labeled as "a") and go to 7 ("h").
 *
 * @return 1 if a piece was found at row and column; 0 otherwise
 */
int fc_board_remove_piece (fc_board_t *board, int row, int col);

/**
 * @brief Assign a new value for the given piece.
 *
 * @param[in,out] board A pointer to the board.
 * @param[in] piece A piece.
 * @param[in] value The new material value for the piece.
 *
 * @return void
 */
void fc_board_set_material_value (fc_board_t *board, fc_piece_t piece,
		int value);

/**
 * @brief Return the material value of a piece.
 *
 * @param[in] board A pointer to the board.
 * @param[in] piece The piece.
 *
 * @return The value of the given piece.
 */
int fc_board_get_material_value (fc_board_t *board, fc_piece_t piece);

/**
 * @brief Adds a move to list.
 *
 * Acts as a wrapper for the fc_mlist_insert() function.  Sets the move's
 * value and then calls fc_mlist_insert().
 *
 * @param[in] board A pointer to the game board.
 * @param[in] list The move list.  The mlist struct should be allocated and
 * initialized before the call to this function.
 * @param[in] move The move to be added to the list.  The move struct is
 * copied into the list, so list will point to a valid copy of move even if
 * move is freed or otherwise deallocated after this call.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_board_list_add_move (fc_board_t *board, fc_mlist_t *list,
		fc_move_t *move);

/**
 * @brief Returns a list of available moves for player.
 *
 * This list represents all available moves for all pieces under player's
 * control.  "Available" means any move that a piece can legally make (which is
 * this instance INCLUDES any move that might put the player's or partner's
 * king in check).
 *
 * @param[in] board A pointer to the game board.
 * @param[out] moves The mlist that the moves will be appended to.  The mlist
 * struct should be allocated and initialized before the call to this function.
 * Additionally, if the mlist has been used previously, you may need to call
 * fc_mlist_clear() on it before this call.
 * @param[in] player The player we want the moves for.
 *
 * @return void
 */
void fc_board_get_all_moves (fc_board_t *board, fc_mlist_t *moves,
			 fc_player_t player);

/**
 * @brief Returns a list of available removes for a player.
 *
 * The function will return a list of removes EVEN IF a player is not actually
 * allowed to remove the piece or any pieces on this move.  Essentially, this
 * is returning a list of all pieces under the player's control.
 *
 * @param[in] board A pointer to the game board.
 * @param[out] moves The mlist that the moves will be appended to.  The mlist
 * struct should be allocated and initialized before the call to this function.
 * Additionally, if the mlist has been used previously, you may need to call
 * fc_mlist_clear() on it before this call.
 * @param[in] player The player we want the removes for.
 *
 * @return void
 */
void fc_board_get_all_removes (fc_board_t *board, fc_mlist_t *moves,
			  fc_player_t player);

/**
 * @brief Determines whether or not the given move will put the player's or
 * partner's king in check.
 *
 * A "valid" move meets the following criteria:
 * 	-# It does not move the player's king into check.
 * 	-# It does not put our partner's king into check (unless he is already
 * 	in check).
 * 	-# If the player's king is in check, then it will move him out of
 * 	check...
 * 	-# unless he's in checkmate; in which case the player may move any
 * 	piece EXCEPT the king.
 *
 * @param[in] board A pointer to the game board.
 * @param[in] move The move we are testing.
 *
 * @return 1 if the move is "valid" as defined above; 0 otherwise
 */
int fc_board_is_move_valid (fc_board_t *board, fc_move_t *move);

/**
 * @brief Return a player's valid moves.
 *
 * This returns all moves that are valid for the player to make.  If there are
 * no moves available to the player, then it will return a list of pieces that
 * the player may remove.
 *
 * @param board A pointer to the board game.
 * @param moves The list of moves that are returned.
 * @param player The player in question.
 *
 * @return void
 */
void fc_board_get_moves (fc_board_t *board, fc_mlist_t *moves,
		fc_player_t player);

/**
 * @brief Determines whether or not the given move requires a pawn to be
 * promoted.
 *
 * @param[in] board A pointer to the game board.
 * @param[in] move The move in question.
 * @param[out] side The orientation of the pawn.
 *
 * @return 1 if the move requires a pawn to be promoted; 0 otherwise
 */
int fc_board_move_requires_promotion (fc_board_t *board, fc_move_t *move,
		fc_player_t *side);

/**
 * @brief Updates the game board with move.
 *
 * @param[in,out] board A pointer to the game board.
 * @param[in] move The move.
 *
 * @return 0 if and only if the move requires a pawn to be promoted, and no
 * promotion piece is specified in the move struct; 1 otherwise
 */
int fc_board_make_move (fc_board_t *board, fc_move_t *move);

/**
 * @brief Updates the game board with move and promotes the pawn.
 *
 * This function can be called if fc_board_make_move() returns 0, and you don't
 * feel like setting the promote value in the move struct.  DO NOT call this
 * function if you do not intend to promote a pawn.
 *
 * @param[in,out] board A pointer to the game board.
 * @param[in] move The move.
 * @param[in] promote The piece to promote the pawn to.
 *
 * @return 1 if the pawn can be moved and promoted to 'promote'; 0 otherwise
 */
int fc_board_make_pawn_move (fc_board_t *board, fc_move_t *move,
			     fc_piece_t promote);

/**
 * @brief Copies the board src to dst.
 *
 * @param[out] dst The destination board.
 * @param[in] src The source board.
 *
 * @return void
 */
void fc_board_copy (fc_board_t *dst, fc_board_t *src);

#ifndef DOXYGEN_IGNORE
#define FC_CHECK 1
#define FC_CHECKMATE 2
#endif /* DOXYGEN_IGNORE */

/**
 * @brief Returns the check status of the player's king.
 *
 * @remark The code for this function resides in src/check.c rather than
 * src/board.c because so much code was written to support it that I felt more
 * comfortable putting it in a separate file.
 *
 * @param[in] board A pointer to the game board.
 * @param[in] player The player whose king we want the check status for.
 *
 * @return 0 if the player's king is not in check; FC_CHECK if the player's
 * king is in check; FC_CHECKMATE if the player's king is in checkmate
 */
int fc_board_check_status (fc_board_t *board, fc_player_t player);

/**
 * @brief Determines whether or not a player has been eliminated from the
 * game.
 *
 * @param board A pointer to the game board.
 * @param player The player in question.
 *
 * @return 1 if the player's king is no longer present in the game; 0
 * otherwise.
 */
int fc_board_is_player_out (fc_board_t *board, fc_player_t player);

/**
 * @brief Determines if one team has reached a winning position.
 *
 * @param board A pointer to the game board.
 *
 * @return 1 if one side has been eliminated; 0 otherwise.
 */
int fc_board_game_over (fc_board_t *board);

/**
 * @brief Determine the relative material worth of a given board configuration.
 *
 * Returns the player's "score".  This number is determined by adding up
 * player's and player's partner's material score and subtracting their
 * opponent's material score.  This gives a basic idea of who is ahead (not
 * taking into account position) in the game.
 *
 * @param[in] board A pointer to the game board.
 * @param[in] player The player we are returning the score for.
 *
 * @return The relative material score as defined above.
 */
int fc_board_score_position (fc_board_t *board, fc_player_t player);

/* TODO */
int fc_board_num_players (fc_board_t *board);

#endif
