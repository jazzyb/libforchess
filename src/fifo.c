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

#include <pthread.h>
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
	if (pthread_mutex_init(&queue->lock, NULL)) {
		free(queue->q);
		return 0;
	}
	return 1;
}

void fc_fifo_free (fc_fifo_t *queue)
{
	free(queue->q);
	pthread_mutex_destroy(&queue->lock);
}

static int is_full (fc_fifo_t *queue)
{
	return queue->count == queue->num_elem;
}

static int is_empty (fc_fifo_t *queue)
{
	return queue->count == 0;
}

int fc_fifo_push (fc_fifo_t *queue, void *item)
{
	unsigned char *dst;

	pthread_mutex_lock(&queue->lock);
	if (is_full(queue)) {
		pthread_mutex_unlock(&queue->lock);
		return 0;
	}

	dst = queue->q + (queue->elem_size * queue->push_index);
	memcpy(dst, item, queue->elem_size);
	queue->push_index = (queue->push_index + 1) % queue->num_elem;
	queue->count += 1;
	pthread_mutex_unlock(&queue->lock);
	return 1;
}

int fc_fifo_pop (fc_fifo_t *queue, void *ret)
{
	unsigned char *src;

	pthread_mutex_lock(&queue->lock);
	if (is_empty(queue)) {
		pthread_mutex_unlock(&queue->lock);
		return 0;
	}

	src = queue->q + (queue->elem_size * queue->pop_index);
	memcpy(ret, src, queue->elem_size);
	queue->pop_index = (queue->pop_index + 1) % queue->num_elem;
	queue->count -= 1;
	pthread_mutex_unlock(&queue->lock);
	return 1;
}

void fc_fifo_clear (fc_fifo_t *queue)
{
	pthread_mutex_lock(&queue->lock);
	queue->count = queue->pop_index = queue->push_index = 0;
	pthread_mutex_unlock(&queue->lock);
}

#define FC_SYNCHRONIZED_RETURN(type, value, mutex_ptr) {\
	type _ret;\
	pthread_mutex_lock(mutex_ptr);\
	_ret = value;\
	pthread_mutex_unlock(mutex_ptr);\
	return _ret;\
}

int fc_fifo_is_full (fc_fifo_t *queue)
{
	FC_SYNCHRONIZED_RETURN(int, is_full(queue), &queue->lock);
}

int fc_fifo_is_empty (fc_fifo_t *queue)
{
	FC_SYNCHRONIZED_RETURN(int, is_empty(queue), &queue->lock);
}

size_t fc_fifo_size (fc_fifo_t *queue)
{
	FC_SYNCHRONIZED_RETURN(size_t, queue->count, &queue->lock);
}

