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

/**
 * @file forchess/threads.h
 * @author jazzyb aka Jason M Barnes
 * @brief Forchess Thread-Pool API
 */

#ifndef DOXYGEN_IGNORE

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

#endif /* DOXYGEN_IGNORE */

/**
 * @brief Initializes a new thread-pool.
 *
 * The fc_tpool_t structure needs to already be allocated before this call.
 *
 * @param pool A pointer to the thread pool.
 * @param num_threads Number of threads to allocate.
 * @param num_tasks Number of tasks that can be queued for the threads to
 * process at any one time.
 *
 * @return 1 if successful; 0 otherwise
 */
int fc_tpool_init (fc_tpool_t *pool, size_t num_threads, size_t num_tasks);

/**
 * @brief Frees the memory allocated by fc_tpool_init.
 *
 * All threads need to be stopped before this call is issued.
 *
 * @param pool A pointer to the thread-pool.
 *
 * @return void
 */
void fc_tpool_free (fc_tpool_t *pool);

/**
 * @brief Tells the threads to begin processing tasks.
 *
 * @param pool A pointer to the thread-pool.
 *
 * @return 1 if the threads were successfully started; 0 otherwise
 */
int fc_tpool_start_threads (fc_tpool_t *pool);

/**
 * @brief Tells the threads to stop processing tasks.
 *
 * @param pool A pointer to the thread-pool.
 *
 * @return 1 if all threads were joined successfully; 0 otherwise
 */
int fc_tpool_stop_threads (fc_tpool_t *pool);

/* FIXME psu_task and pop_results should block until the task/result can be
 * (de)queued to/from the thread-pool.  Don't do this return 0 on block
 * bullshit by default. */
/**
 * @brief Push a task onto the thread-pool to be processed.
 *
 * A task is given as a callback pointer, a pointer to some input structure,
 * and a pointer to an output structure.  The input structure should contain
 * any variables that the callback function needs to process.  The results
 * should be placed in the output structure by the callback.  When the
 * callback has finished, the output structure will be placed on a result
 * queue (see fc_tpool_pop_result below).  The thread-pool will not allocate
 * memory for the input or output data-structures, so these pointers must
 * point to valid memory until the result is popped form the pool.
 *
 * @param pool A pointer to the thread-pool.
 * @param callback A pointer to a routine to be executed by one of the
 * threads.
 * @param input_data A pointer to a structure that will be processed by the
 * callback; may be NULL.
 * @param output_data A pointer to a structure in which the callback function
 * may place any return values; may be NULL.
 *
 * @return a "task id" (ret > 0) if the task is successfully placed in the
 * thread-pool.  0 is returned if the task queue is full or the queue was
 * blocking.  If this is the case, the user may try placing the task on the
 * queue again.
 */
int fc_tpool_push_task (fc_tpool_t *pool,
		void (*callback) (void *input_data, void *output_data),
		void *input_data, void *output_data);

/* FIXME Allow NULL for output_data - meaning we don't care about the return
 * value. */
/**
 * @brief Pop a result from the thread-pool.
 *
 * @param pool A pointer to the thread-pool.
 * @param output_data Changes *output_data to point to the output_data from
 * the task that was placed on the queue.
 *
 * @return the appropriate task id that was returned when the task was pushed
 * to the thread-pool. 0 is returned if the thread-pool was blocking.
 */
int fc_tpool_pop_result (fc_tpool_t *pool, void **output_data);

/**
 * @brief Delete all pending tasks and drop all currently running tasks.
 *
 * @param pool A pointer to the thread-pool.
 *
 * @return void
 */
void fc_tpool_clear_tasks (fc_tpool_t *pool);

/**
 * @brief Returns the number of threads in the thread-pool.
 *
 * @param pool A pointer to the thread-pool.
 *
 * @return number of threads
 */
size_t fc_tpool_size (fc_tpool_t *pool);

/**
 * @brief Returns the number of threads that are not processing a task.
 *
 * @param pool A pointer to the thread-pool.
 *
 * @return number of idle threads
 */
size_t fc_tpool_num_idle_threads (fc_tpool_t *pool);

/**
 * @brief Returns the number of threads that are currently busy processing
 * tasks.
 *
 * @param pool A pointer to the thread-pool.
 *
 * @return number of busy threads
 */
size_t fc_tpool_num_busy_threads (fc_tpool_t *pool);

/**
 * @brief Returns number of tasks pending processing.
 *
 * @param pool A pointer to the thread-pool.
 *
 * @return number of pending tasks
 */
size_t fc_tpool_num_pending_tasks (fc_tpool_t *pool);

/**
 * @brief Returns the number of results that have been processed.
 *
 * @param pool A pointer to the thread-pool.
 *
 * @return number of processed results
 */
size_t fc_tpool_num_pending_results (fc_tpool_t *pool);

#endif
