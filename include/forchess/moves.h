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

#ifndef _FC_MOVES_H_
#define _FC_MOVES_H_

/**
 * @file forchess/moves.h
 * @author jazzyb aka Jason M Barnes
 * @brief API for manipulating the data-structures which hold forchess moves.
 */

#ifndef DOXYGEN_IGNORE

#include <stdint.h>

#define FC_NONE (-1)

typedef enum {
	FC_FIRST = 0,
	FC_SECOND,
	FC_THIRD,
	FC_FOURTH
} fc_player_t;
#define FC_NUM_PLAYERS 4

#define FC_NEXT_PLAYER(player) ((player + 1) % 4)
#define FC_PARTNER(player) ((player + 2) % 4)

typedef enum {
	FC_PAWN = 0,
	FC_BISHOP,
	FC_KNIGHT,
	FC_ROOK,
	FC_QUEEN,
	FC_KING
} fc_piece_t;
#define FC_NUM_PIECES (FC_KING + 1)

typedef struct {
	fc_player_t player;
	fc_piece_t piece;
	fc_player_t opp_player;
	fc_piece_t opp_piece;
	fc_piece_t promote;
	uint64_t move;
	int32_t value;
} fc_move_t;

#define FC_DEFAULT_MLIST_SIZE 255

typedef struct {
	fc_move_t *moves;
	uint8_t index;
	uint8_t *sorted;
} fc_mlist_t;

typedef struct {
	fc_mlist_t *list;
	void *data;
	int current;
	fc_move_t *(*callback) (void *data, fc_mlist_t *list, int *current);
} fc_mlist_iter_t;

uint64_t fc_uint64(const char *move);

#endif /* DOXYGEN_IGNORE */

/**
 * @brief Copy the move src to dst.
 *
 * @param[out] dst The destination move.
 * @param[in] src The source move.
 *
 * @return void
 */
void fc_move_copy (fc_move_t *dst, fc_move_t *src);

/**
 * @brief Set the promotion piece in move.
 *
 * @param[out] move The move which requires promotion.
 * @param[in] promote The piece to promote the pawn to.
 *
 * @return void
 */
void fc_move_set_promotion (fc_move_t *move, fc_piece_t promote);

/**
 * @brief Initialize an mlist.
 *
 * This function allocates memory for the internal data-structures of the mlist
 * struct.  This function MUST be called before calling any of the other
 * fc_mlist_* functions.  The space for list must have already been allocated
 * before the call to this function.
 *
 * @param[out] list The new list.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_mlist_init (fc_mlist_t *list);

/**
 * @brief Copies the list dst to src.
 *
 * The mlist dst should have already been initialized before this call.
 *
 * @param[out] dst The destination mlist.
 * @param[in] src The source mlist.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_mlist_copy (fc_mlist_t *dst, fc_mlist_t *src);

/**
 * @brief Inserts move into list based on value.
 *
 * After successive calls to fc_mlist_insert(), the list will hold the moves
 * in DESCENDING order based on value.  For example, if moves are inserted in
 * the following order with the values (4, 5, 2, 1, 3).  The moves in the
 * mlist will be ordered as (5, 4, 3, 2, 1).
 *
 * @param list The move list.
 * @param move The move to be inserted.
 * @param value The "value" of the move.
 *
 * @return 1 on successful insertion; 0 otherwise
 */
int fc_mlist_insert (fc_mlist_t *list, fc_move_t *move, int32_t value);

/**
 * @brief Merges two lists together.
 *
 * fc_mlist_merge() maintains a sorted order.  The moves in src will be placed
 * in dst.  See fc_mlist_insert() for a description of the sorted order of the
 * moves.
 *
 * @param dst The list in which to insert the new moves.
 * @param src The source of the new moves.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_mlist_merge (fc_mlist_t *dst, fc_mlist_t *src);

/**
 * @brief Delete the move at index from the list.
 *
 * fc_mlist_delete() maintains sorted order.  See fc_mlist_insert() for a
 * description of the sorted order of the moves.
 */
int fc_mlist_delete (fc_mlist_t *list, int index);

/**
 * @brief De-initializes the mlist.
 *
 * If list was dynamically allocated by the user, then the user must free list
 * after this call.
 *
 * @param[out] list The list to be freed.
 *
 * @return void
 */
void fc_mlist_free (fc_mlist_t *list);

/**
 * @brief Returns the number of moves that mlist contains.
 *
 * @param[in] list The list of moves.
 *
 * @return the number of moves in list
 */
int fc_mlist_length (fc_mlist_t *list);

/**
 * @brief Deletes the current moves in the mlist.
 *
 * One may use this function to remove all the moves in a list without
 * resorting to calling fc_mlist_free() followed by fc_mlist_init().  This
 * function does not allocate or free any memory itself, so it's much faster
 * than calling the other functions as well as easier.
 *
 * @param[out] list the list of moves to be removed.
 *
 * @return void
 */
void fc_mlist_clear (fc_mlist_t *list);

/**
 * @brief Returns a pointer to the move at the given index in list.
 *
 * @param[in] list The list of moves.
 * @param[in] index The index into the moves.
 *
 * @return a pointer to the move at index
 */
fc_move_t *fc_mlist_get (fc_mlist_t *list, int index);

/* TODO */
int fc_mlist_iter_init (fc_mlist_iter_t *mliter, fc_mlist_t *list, void *data,
		fc_move_t *(*callback) (void *data, fc_mlist_t *list,
			int *current));

fc_move_t *fc_mlist_iter_next (fc_mlist_iter_t *mliter);

void fc_mlist_iter_free (fc_mlist_iter_t *mliter);

#endif
