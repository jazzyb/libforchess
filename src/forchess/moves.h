/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
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
} fc_move_t;

typedef struct {
	fc_move_t *moves;
	int size;
	int index;
} fc_mlist_t;

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
 * @param[in] size The number of moves that will be allocated in the list by
 * default.  If the size is less than 1, then the default of 130 moves will be
 * used.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_mlist_init (fc_mlist_t *list, int size);

/**
 * @brief Resize an mlist.
 *
 * Reset the number of moves that the given mlist may hold.
 *
 * @param[out] list The list to be resized.
 * @param[in] new_size The new number of moves that the list may hold.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_mlist_resize (fc_mlist_t *list, int new_size);

/**
 * @brief Append a new move to the mlist.
 *
 * This function copies the move parameter onto the end of the mlist.  So, for
 * example, the user may free the move struct after the call, and the end of
 * the mlist will still point to a valid move.
 *
 * @note The mlist will automatically resize itself if it doesn't have enough
 * room for the new move.
 *
 * @param[out] list The list of moves.
 * @param[in] move The new move.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_mlist_append (fc_mlist_t *list, fc_move_t *move);

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
 * @brief Concatenates src onto the end of dst.
 *
 * @param[out] dst The destination mlist.
 * @param[in] src The source mlist.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_mlist_cat  (fc_mlist_t *dst, fc_mlist_t *src);

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

#endif
