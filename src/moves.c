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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "forchess/board.h"
#include "forchess/moves.h"

/* NOTE: no sanitization of string */
uint64_t fc_uint64 (const char *str)
{
	char x1, x2, y1, y2;
	sscanf(str, "%c%c-%c%c", &x1, &y1, &x2, &y2);
	x1 -= 'a';
	x2 -= 'a';
	y1 -= '1';
	y2 -= '1';

	return (((uint64_t)1) << ((y1 * 8) + x1)) |
	       (((uint64_t)1) << ((y2 * 8) + x2));
}

void fc_move_copy (fc_move_t *dst, fc_move_t *src)
{
	memcpy(dst, src, sizeof(*dst));
}

void fc_move_set_promotion (fc_move_t *move, fc_piece_t promote)
{
	move->promote = promote;
}

/*
 * This must be called before the insert, copy, and merge functions can be
 * used.
 */
int fc_mlist_init (fc_mlist_t *list)
{
	list->index = 0;

	list->moves = calloc(FC_DEFAULT_MLIST_SIZE, sizeof(*list->moves));
	if (!list->moves) {
		return 0;
	}

	return 1;
}

int fc_mlist_copy (fc_mlist_t *dst, fc_mlist_t *src)
{
	uint32_t i;

	for (i = 0; i < src->index; i++) {
		fc_move_copy(dst->moves + i, src->moves + i);
	}
	dst->index = src->index;
	return 1;
}

/*
 * The below two functions represent a very naive sorting algorithm, but it
 * seems to be "fast enough" for the time being.
 *
 * NOTE:  A sorted mlist is in DESC order by move.value.  For example,
 * (5, 3, 2, 4, 1) would become (5, 4, 3, 2, 1).
 */
int fc_mlist_insert (fc_mlist_t *list, fc_move_t *move, int32_t value)
{
	uint32_t i;
	fc_move_t *old;

	assert(list->index + 1 <= FC_DEFAULT_MLIST_SIZE);

	/*
	 * TODO binary search might be faster
	 */
	for (i = 0; i < list->index; i++) {
		old = list->moves + i;
		if (value > old->value) {
			break;
		}
	}
	(void)memmove(list->moves + i + 1, list->moves + i,
			(list->index - i) * sizeof(*list->moves));
	fc_move_copy(list->moves + i, move);
	list->moves[i].value = value;
	list->index += 1;
	return 1;
}

/*
 * NOTE:  We may be able to speed this function up if we need to by *not*
 * removing the move structs and keeping up with two different indices: one
 * for the moves and another for the sorted index.
 *
 * Need to profile and see if the change is worth it.
 */
int fc_mlist_delete (fc_mlist_t *list, int index)
{
	if (index < 0 || index >= list->index) {
		return 0;
	}

	memmove(list->moves + index, list->moves + index + 1,
			(list->index - index) * sizeof(*list->moves));
	list->index -= 1;
	return 1;
}

/*
 * NOTE: Assumes that the values are initialized for all moves in both lists.
 */
int fc_mlist_merge (fc_mlist_t *dst, fc_mlist_t *src)
{
	uint32_t i;
	fc_move_t *move;

	assert((uint64_t)dst->index + (uint64_t)src->index <=
			FC_DEFAULT_MLIST_SIZE);

	for (i = 0; i < src->index; i++) {
		move = fc_mlist_get(src, i);
		fc_mlist_insert(dst, move, move->value);
	}
	return 1;
}

/*
 * This function frees the array of moves pointed to in the structure.  If the
 * user has malloced the fc_mlist_t structure, then they must call free on that
 * themselves.
 */
void fc_mlist_free (fc_mlist_t *list)
{
	free(list->moves);
	list->index = 0;
}

/*
 * Return the length of the list.
 */
int fc_mlist_length (fc_mlist_t *list)
{
	return list->index;
}

/*
 * Clear the entries in the list so it can be reused.
 */
void fc_mlist_clear (fc_mlist_t *list)
{
	list->index = 0;
}

/*
 * Return a pointer to the move at index.
 */
fc_move_t *fc_mlist_get (fc_mlist_t *list, int index)
{
	if (index >= list->index) {
		return NULL;
	}
	return list->moves + index;
}

int fc_mlist_iter_init (fc_mlist_t *list, fc_mlist_iter_t *iter,
		fc_move_t *(*callback) (fc_mlist_iter_t *iter))
{
	iter->list = list;
	iter->current_index = 0;
	iter->state = NULL;
	iter->move = NULL;
	iter->callback = callback;
	return 1;
}

fc_mlist_iter_t *fc_mlist_iter_next (fc_mlist_iter_t *iter)
{
	/*
	 * We check for current_index being 0 because we could have the
	 * situation where a user passes in an empty list intending for the
	 * callback to fill it, so we want to call the callback at least once.
	 */
	if (iter->current_index != 0 &&
			iter->current_index >= fc_mlist_length(iter->list)) {
		return NULL;
	}
	iter->move = iter->callback(iter);
	if (iter->move == NULL) {
		return NULL;
	}
	iter->current_index += 1;
	return iter;
}

fc_move_t *fc_mlist_iter_get_move (fc_mlist_iter_t *iter)
{
	return iter->move;
}

void *fc_mlist_iter_get_state (fc_mlist_iter_t *iter)
{
	return iter->state;
}

void fc_mlist_iter_set_state (fc_mlist_iter_t *iter, void *state)
{
	iter->state = state;
}

fc_mlist_t *fc_mlist_iter_get_mlist (fc_mlist_iter_t *iter)
{
	return iter->list;
}

int fc_mlist_iter_get_index (fc_mlist_iter_t *iter)
{
	return iter->current_index;
}

void fc_mlist_iter_set_index (fc_mlist_iter_t *iter, int index)
{
	iter->current_index = index;
}

