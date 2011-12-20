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

#ifndef _FC_FIFO_H_
#define _FC_FIFO_H_

typedef struct {
	unsigned char *q;
	size_t num_elem;
	size_t elem_size;
	size_t count;
	size_t push_index;
	size_t pop_index;
} fc_fifo_t;

/* TODO: Documentation */
int fc_fifo_init (fc_fifo_t *queue, size_t count, size_t size);
int fc_fifo_push (fc_fifo_t *queue, void *item);
int fc_fifo_pop (fc_fifo_t *queue, void *ret);
int fc_fifo_is_full (fc_fifo_t *queue);
int fc_fifo_is_empty (fc_fifo_t *queue);
void fc_fifo_free (fc_fifo_t *queue);

#endif
