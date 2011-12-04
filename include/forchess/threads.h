/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 */

#ifndef _FC_THREADS_H_
#define _FC_THREADS_H_

#include <pthread.h>

#include "forchess/fifo.h"

typedef struct {
	int id;
	void (*callback) (void *, void *);
	void *input_data;
	void *output_data;
} fc_task_t;

typedef struct {
	pthread_t *threads;
	pthread_attr_t attr;
	pthread_mutex_t mutex;
	fc_fifo_t taskq;
	fc_fifo_t resultq;
	size_t num_threads;
	int last_id;
	int die;
} fc_tpool_t;

int fc_tpool_init (fc_tpool_t *pool, size_t num_threads);
int fc_tpool_start_threads (fc_tpool_t *pool);
int fc_tpool_stop_threads (fc_tpool_t *pool);
int fc_tpool_push_task (fc_tpool_t *pool,
		void (*callback) (void *input_data, void *output_data),
		void *input_data, void *output_data);
int fc_tpool_pop_result (fc_tpool_t *pool, void **output_data);
int fc_tpool_kill_threads (fc_tpool_t *pool);
void fc_tpool_free (fc_tpool_t *pool);

#endif
