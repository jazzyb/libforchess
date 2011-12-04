#include <stdlib.h>
#include <string.h>

#include "forchess/fifo.h"

int fc_fifo_init (fc_fifo_t *queue, size_t count, size_t size)
{
	queue->num_elem = count;
	queue->elem_size = size;
	queue->q = calloc(queue->num_elem, queue->elem_size);
	if (!queue->q) {
		return 0;
	}
	queue->index = 0;
	return 1;
}

void fc_fifo_free (fc_fifo_t *queue)
{
	free(queue->q);
}

int fc_fifo_is_full (fc_fifo_t *queue)
{
	return queue->index == queue->num_elem;
}

int fc_fifo_is_empty (fc_fifo_t *queue)
{
	return queue->index == 0;
}

int fc_fifo_push (fc_fifo_t *queue, void *item)
{
	unsigned char *p;

	if (fc_fifo_is_full(queue)) {
		return 0;
	}

	p = queue->q + (queue->elem_size * queue->index);
	memcpy(p, item, queue->elem_size);
	queue->index += 1;
	return 1;
}

int fc_fifo_pop (fc_fifo_t *queue, void *ret)
{
	unsigned char *p;

	if (fc_fifo_is_empty(queue)) {
		return 0;
	}

	memcpy(ret, queue->q, queue->elem_size);
	queue->index -= 1;
	p = queue->q + queue->elem_size;
	memmove(queue->q, p, queue->elem_size * queue->index);
	return 1;
}

