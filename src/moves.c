/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 */

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

/* Redefine UINT64_C to be something that plays nicer with the C89 standard */
#undef UINT64_C
#define UINT64_C(x) ((uint64_t)x)

	return ((UINT64_C(1)) << ((y1 * 8) + x1)) |
	       ((UINT64_C(1)) << ((y2 * 8) + x2));
}

void fc_move_copy (fc_move_t *dst, fc_move_t *src)
{
	memcpy(dst, src, sizeof(*dst));
}

void fc_move_set_promotion (fc_move_t *move, fc_piece_t promote)
{
	move->promote = promote;
}

#define FC_DEFAULT_MLIST_SIZE 130 /* totally arbitrary */

/*
 * This must be called before the append, copy, and cat functions can be used.
 */
int fc_mlist_init (fc_mlist_t *list, uint32_t size)
{
	list->index = 0;

	if (size > 0) {
		list->size = size;
	} else {
		list->size = FC_DEFAULT_MLIST_SIZE;
	}

	list->moves = calloc(list->size, sizeof(*list->moves));
	if (!list->moves) {
		return 0;
	}
	list->sorted = calloc(list->size, sizeof(*list->sorted));
	if (!list->sorted) {
		free(list->moves);
		list->moves = NULL;
		return 0;
	}

	return 1;
}

int fc_mlist_resize (fc_mlist_t *list, uint32_t new_size)
{
	uint32_t *new_sorted;
	fc_move_t *new_moves;

	if (new_size < list->index) {
		return 0;
	}

	new_moves = realloc(list->moves, new_size * sizeof(*new_moves));
	if (!new_moves) {
		return 0;
	}
	new_sorted = realloc(list->sorted, new_size * sizeof(*new_sorted));
	if (!new_sorted) {
		free(new_moves);
		return 0;
	}
	list->moves = new_moves;
	list->sorted = new_sorted;
	list->size = new_size;
	return 1;
}

int fc_mlist_copy (fc_mlist_t *dst, fc_mlist_t *src)
{
	uint32_t i;

	if (dst->size < src->index) {
		if (!fc_mlist_resize(dst, src->size)) {
			return 0;
		}
	}

	for (i = 0; i < src->index; i++) {
		fc_move_copy(dst->moves + i, src->moves + i);
		dst->sorted[i] = src->sorted[i];
		dst->index += 1;
	}

	return 1;
}

/*
 * NOTE:  A sorted mlist is in DESC order by move.value.  For example,
 * (5, 3, 2, 4, 1) would become (5, 4, 3, 2, 1).
 */
int fc_mlist_sort (fc_mlist_t *list)
{
	/* FIXME TODO */
	return 1;
}

int fc_mlist_insert (fc_mlist_t *list, fc_move_t *move)
{
	uint32_t i;
	fc_move_t *new, *old;

	if (list->index >= list->size) {
		if (!fc_mlist_resize(list, list->size * 2)) {
			return 0;
		}
	}

	new = &(list->moves[list->index]);
	fc_move_copy(new, move);

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

int fc_mlist_merge (fc_mlist_t *dst, fc_mlist_t *src)
{
	/* TODO FIXME */
	return 0;
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
	list->index = list->size = 0;
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

