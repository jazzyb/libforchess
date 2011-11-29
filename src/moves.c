/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define FC_DEFAULT_MLIST_SIZE 255

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
	list->sorted = calloc(FC_DEFAULT_MLIST_SIZE, sizeof(*list->sorted));
	if (!list->sorted) {
		free(list->moves);
		list->moves = NULL;
		return 0;
	}

	return 1;
}

int fc_mlist_copy (fc_mlist_t *dst, fc_mlist_t *src)
{
	uint32_t i;

	for (i = 0; i < src->index; i++) {
		fc_move_copy(dst->moves + i, src->moves + i);
		dst->sorted[i] = src->sorted[i];
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
	fc_move_t *new, *old;

	new = &(list->moves[list->index]);
	fc_move_copy(new, move);
	new->value = value;

	/*
	 * TODO binary search might be faster
	 */
	for (i = 0; i < list->index; i++) {
		old = list->moves + list->sorted[i];
		if (new->value > old->value) {
			break;
		}
	}
	(void)memmove(list->sorted + i + 1, list->sorted + i,
			(list->index - i) * sizeof(int32_t));
	list->sorted[i] = list->index;
	list->index += 1;
	return 1;
}

/*
 * NOTE: Assumes that the values are initialized for all moves in both lists.
 */
int fc_mlist_merge (fc_mlist_t *dst, fc_mlist_t *src)
{
	uint8_t i;
	fc_move_t *move;

	assert((uint32_t)dst->index + (uint32_t)src->index <=
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
	free(list->sorted);
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
	return list->moves + list->sorted[index];
}

