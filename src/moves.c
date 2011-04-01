#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
	return ((UINT64_C(1)) << (y1 * 8) + x1) |
	       ((UINT64_C(1)) << (y2 * 8) + x2);
}

void fc_move_copy (fc_move_t *dst, fc_move_t *src)
{
	dst->player = src->player;
	dst->piece = src->piece;
	dst->promote = src->promote;
	dst->move = src->move;
}

void fc_move_set_promotion (fc_move_t *move, fc_piece_t promote)
{
	move->promote = promote;
}

#define FC_DEFAULT_MLIST_SIZE 130 /* totally arbitrary */

/*
 * This must be called before the append, copy, and cat functions can be used.
 */
int fc_mlist_init (fc_mlist_t *list, int size)
{
	list->index = 0;

	if (size > 0) {
		list->size = size;
	} else {
		list->size = FC_DEFAULT_MLIST_SIZE;
	}

	list->moves = calloc(list->size, sizeof(fc_move_t));
	if (!list->moves) {
		return 0;
	}
	return 1;
}

int fc_mlist_resize (fc_mlist_t *list, int new_size)
{
	if (new_size < list->index) {
		return 0;
	}

	fc_move_t *ok = realloc(list->moves, new_size * sizeof(fc_move_t));
	if (!ok) {
		return 0;
	}
	list->moves = ok;
	list->size = new_size;
	return 1;
}

int fc_mlist_append (fc_mlist_t *list, fc_player_t player, fc_piece_t piece,
		     fc_piece_t promote, uint64_t move)
{
	if (list->index >= list->size) {
		if (!fc_mlist_resize(list, list->size * 2)) {
			return 0;
		}
	}

	list->moves[list->index].player = player;
	list->moves[list->index].piece = piece;
	list->moves[list->index].promote = promote;
	list->moves[list->index].move = move;
	list->index += 1;
	return 1;
}

int fc_mlist_copy (fc_mlist_t *dst, fc_mlist_t *src)
{
	if (dst->size < src->index) {
		if (!fc_mlist_resize(dst, src->size)) {
			return 0;
		}
	}

	for (int i = 0; i < src->index; i++) {
		fc_move_copy(dst->moves + i, src->moves + i);
		dst->index += 1;
	}

	return 1;
}

int fc_mlist_cat (fc_mlist_t *dst, fc_mlist_t *src)
{
	int resize_flag = 0;

	while (dst->index + src->index > dst->size) {
		resize_flag = 1;
		dst->size *= 2;
	}
	if (resize_flag && !fc_mlist_resize(dst, dst->size)) {
		return 0;
	}

	for (int i = 0; i < src->index; i++) {
		fc_move_copy(dst->moves + dst->index, src->moves + i);
		dst->index += 1;
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
	//bzero(list->moves, list->size * sizeof(fc_move_t));
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
