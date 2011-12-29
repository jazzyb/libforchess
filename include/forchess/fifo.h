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

/**
 * @file forchess/fifo.h
 * @author jazzyb aka Jason M Barnes
 * @brief Forchess Queue API
 */

#ifndef DOXYGEN_IGNORE

#include <pthread.h>

typedef struct {
	unsigned char *q;
	size_t num_elem;
	size_t elem_size;
	size_t count;
	size_t push_index;
	size_t pop_index;
	pthread_mutex_t lock;
} fc_fifo_t;

#endif /* DOXYGEN_IGNORE */

/**
 * @brief Initialize a queue.
 *
 * @param queue A pointer to the queue.  The space should already be
 * allocated.
 * @param count The maximum number of items to be held in the queue.
 * @param size The size of each item to be held in the queue.
 *
 * @return 1 if initialization was successful; 0 on failure.
 */
int fc_fifo_init (fc_fifo_t *queue, size_t count, size_t size);

/**
 * @brief Push an item on the queue.
 *
 * @param queue A pointer to the queue.
 * @param item A pointer to the item to be pushed on the queue.  The item is
 * copied from the memory pointed to by item onto the queue, so the actual
 * item may be freed or reused if required.
 * 
 * @return 1 if the item was successfully pushed on the queue; 0 if the queue
 * is full
 */
int fc_fifo_push (fc_fifo_t *queue, void *item);

/**
 * @brief Dequeue an item from the queue.
 *
 * @param queue A pointer to the queue.
 * @param ret The returned item.  ret should pointer to valid memory; the
 * routine copies the item from the queue to ret.
 *
 * @return 1 if the item was successfully dequeued; 0 if the queue was empty.
 */
int fc_fifo_pop (fc_fifo_t *queue, void *ret);

/**
 * @brief Delete all items on the queue.
 *
 * @param queue A pointer to the queue.
 *
 * @return void
 */
void fc_fifo_clear (fc_fifo_t *queue);

/**
 * @brief Queries whether or not the queue is full.
 *
 * @param queue A pointer to the queue.
 *
 * @return 1 if the queue is full; 0 otherwise.
 */
int fc_fifo_is_full (fc_fifo_t *queue);

/**
 * @brief Queries whether or not the queue is empty.
 *
 * @param queue A pointer to the queue.
 *
 * @return 1 if the queue is empty; 0 otherwise.
 */
int fc_fifo_is_empty (fc_fifo_t *queue);

/**
 * @brief Returns the number of items currently in the queue.
 *
 * @param queue A pointer to the queue.
 *
 * @return the number of items in the queue
 */
size_t fc_fifo_size (fc_fifo_t *queue);

/**
 * @brief Frees the memory allocated in the queue by fc_fifo_init.
 *
 * @param queue A pointer to the queue.
 *
 * @return void
 */
void fc_fifo_free (fc_fifo_t *queue);

#endif
