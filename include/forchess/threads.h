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

typedef struct tpool {
	pthread_t *threads;
	pthread_attr_t attr;
	pthread_mutex_t mutex;
	fc_fifo_t taskq;
	fc_fifo_t resultq;
	size_t num_threads;
	int idle_thread_count;
	int last_id;
	int die;
	/*
	 * Each thread has their own flag to indicate if they should discard
	 * their current task.  See fc_tpool_clear_tasks().
	 */
	int *discard_task;
} fc_tpool_t;

int fc_tpool_init (fc_tpool_t *pool, size_t num_threads, size_t num_tasks);
void fc_tpool_free (fc_tpool_t *pool);

int fc_tpool_start_threads (fc_tpool_t *pool);
int fc_tpool_stop_threads (fc_tpool_t *pool);
int fc_tpool_kill_threads (fc_tpool_t *pool);

int fc_tpool_push_task (fc_tpool_t *pool,
		void (*callback) (void *input_data, void *output_data),
		void *input_data, void *output_data);
int fc_tpool_pop_result (fc_tpool_t *pool, void **output_data);
void fc_tpool_clear_tasks (fc_tpool_t *pool);

size_t fc_tpool_size (fc_tpool_t *pool);
size_t fc_tpool_num_idle_threads (fc_tpool_t *pool);
size_t fc_tpool_num_busy_threads (fc_tpool_t *pool);
size_t fc_tpool_num_pending_tasks (fc_tpool_t *pool);
size_t fc_tpool_num_pending_results (fc_tpool_t *pool);

#endif
