/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 */

#include <stdlib.h>
#include <string.h>

#include "forchess/fifo.h"

int fc_fifo_init (fc_fifo_t *queue, size_t num_elem, size_t size)
{
	queue->num_elem = num_elem;
	queue->elem_size = size;
	queue->q = calloc(queue->num_elem, queue->elem_size);
	if (!queue->q) {
		return 0;
	}
	queue->count = queue->push_index = queue->pop_index = 0;
	return 1;
}

void fc_fifo_free (fc_fifo_t *queue)
{
	free(queue->q);
}

int fc_fifo_is_full (fc_fifo_t *queue)
{
	return queue->count == queue->num_elem;
}

int fc_fifo_is_empty (fc_fifo_t *queue)
{
	return queue->count == 0;
}

int fc_fifo_push (fc_fifo_t *queue, void *item)
{
	unsigned char *dst;

	if (fc_fifo_is_full(queue)) {
		return 0;
	}

	dst = queue->q + (queue->elem_size * queue->push_index);
	memcpy(dst, item, queue->elem_size);
	queue->push_index = (queue->push_index + 1) % queue->num_elem;
	queue->count += 1;
	return 1;
}

int fc_fifo_pop (fc_fifo_t *queue, void *ret)
{
	unsigned char *src;

	if (fc_fifo_is_empty(queue)) {
		return 0;
	}

	src = queue->q + (queue->elem_size * queue->pop_index);
	memcpy(ret, src, queue->elem_size);
	queue->pop_index = (queue->pop_index + 1) % queue->num_elem;
	queue->count -= 1;
	return 1;
}

