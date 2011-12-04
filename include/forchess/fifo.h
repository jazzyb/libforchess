/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 */

#ifndef _FC_FIFO_H_
#define _FC_FIFO_H_

typedef struct {
	unsigned char *q;
	size_t num_elem;
	size_t elem_size;
	size_t index;
} fc_fifo_t;

/* TODO: Documentation */
int fc_fifo_init (fc_fifo_t *queue, size_t count, size_t size);
int fc_fifo_push (fc_fifo_t *queue, void *item);
int fc_fifo_pop (fc_fifo_t *queue, void *ret);
int fc_fifo_is_full (fc_fifo_t *queue);
int fc_fifo_is_empty (fc_fifo_t *queue);
void fc_fifo_free (fc_fifo_t *queue);

#endif
