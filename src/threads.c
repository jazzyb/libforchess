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

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "forchess/threads.h"

int fc_tpool_init (fc_tpool_t *pool, size_t num_threads, size_t num_tasks)
{
	pool->num_threads = num_threads;
	pool->threads = calloc(num_threads, sizeof(pthread_t));
	if (!pool->threads) {
		return 0;
	}
	pthread_attr_init(&pool->attr);
	pthread_attr_setdetachstate(&pool->attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&pool->mutex, NULL);
	if (!fc_fifo_init(&pool->taskq, num_tasks, sizeof(fc_task_t))) {
		free(pool->threads);
		pthread_mutex_destroy(&pool->mutex);
		return 0;
	}
	if (!fc_fifo_init(&pool->resultq, num_tasks, sizeof(fc_task_t))) {
		free(pool->threads);
		pthread_mutex_destroy(&pool->mutex);
		fc_fifo_free(&pool->taskq);
		return 0;
	}
	pool->last_id = 0;
	pool->die = 0;
	pool->discard_task = calloc(num_threads, sizeof(int));
	if (!pool->discard_task) {
		free(pool->threads);
		pthread_mutex_destroy(&pool->mutex);
		fc_fifo_free(&pool->taskq);
		fc_fifo_free(&pool->resultq);
		return 0;
	}
	return 1;
}

/*
 * WARNING: Only call this after the thread pool has been stopped.
 */
void fc_tpool_free (fc_tpool_t *pool)
{
	pthread_mutex_destroy(&pool->mutex);
	fc_fifo_free(&pool->taskq);
	fc_fifo_free(&pool->resultq);
	free(pool->threads);
	free(pool->discard_task);
}

struct thread_data {
	int thread_id;
	fc_tpool_t *pool;
	struct itimerval itimer;
	pthread_cond_t ready;
	pthread_mutex_t mutex;
};

/*
 * Called from the start of fc_thread_routine to initialize the necessary
 * variables.
 */
static void initialize_thread_routine (struct thread_data *tdp, int *id,
		fc_tpool_t **pool)
{
	pthread_mutex_lock(&tdp->mutex);
	*id = tdp->thread_id;
	*pool = tdp->pool;
	setitimer(ITIMER_PROF, &tdp->itimer, NULL);
	pthread_cond_signal(&tdp->ready);
	pthread_mutex_unlock(&tdp->mutex);
}

/*
 * Returns 1 if it successfully pulled a new task from the taskq.  Returns 0
 * if the thread must shutdown.
 */
static int pull_task_from_queue (int id, fc_task_t *task, fc_tpool_t *pool)
{
	do {
		pthread_mutex_lock(&pool->mutex);
		if (pool->die) {
			pthread_mutex_unlock(&pool->mutex);
			return 0;
		}
		if (pool->discard_task[id]) {
			pool->discard_task[id] = 0;
		}
		pthread_mutex_unlock(&pool->mutex);
	} while (!fc_fifo_pop(&pool->taskq, task));

	pthread_mutex_lock(&pool->mutex);
	assert(pool->idle_thread_count > 0);
	pool->idle_thread_count -= 1;
	pthread_mutex_unlock(&pool->mutex);
	return 1;
}

/*
 * Tries to push the task on to the resultq.  Returns 0 if the thread must
 * shutdown; otherwise 1.
 */
static int push_result_to_queue (int id, fc_task_t *task, fc_tpool_t *pool)
{
	do {
		pthread_mutex_lock(&pool->mutex);
		if (pool->die) {
			pthread_mutex_unlock(&pool->mutex);
			return 0;
		}
		if (pool->discard_task[id]) {
			pool->discard_task[id] = 0;
			pthread_mutex_unlock(&pool->mutex);
			break;
		}
		pthread_mutex_unlock(&pool->mutex);
	} while (!fc_fifo_push(&pool->resultq, task));

	pthread_mutex_lock(&pool->mutex);
	assert(pool->idle_thread_count != pool->num_threads);
	pool->idle_thread_count += 1;
	pthread_mutex_unlock(&pool->mutex);
	return 1;
}

static void *fc_thread_routine (void *data)
{
	int id;
	fc_task_t task;
	fc_tpool_t *pool;

	initialize_thread_routine((struct thread_data *)data, &id, &pool);

	/* pull tasks from the queue and run them until we are told to die */
	while (1) {
		if (!pull_task_from_queue(id, &task, pool)) {
			break;
		}

		task.callback(task.input_data, task.output_data);

		if (!push_result_to_queue(id, &task, pool)) {
			break;
		}
	}

	pthread_exit(NULL);
}

int fc_tpool_start_threads (fc_tpool_t *pool)
{
	int i, rc;
	struct thread_data data;

	pthread_cond_init(&data.ready, NULL);
	pthread_mutex_init(&data.mutex, NULL);
	pthread_mutex_lock(&pool->mutex);
	pool->die = 0;
	data.pool = pool;
	for (i = 0; i < pool->num_threads; i++) {
		data.thread_id = i;
		getitimer(ITIMER_PROF, &data.itimer);
		rc = pthread_create(&pool->threads[i], &pool->attr,
				&fc_thread_routine, &data);
		if (rc != 0) {
			pool->die = 1;
			pthread_mutex_unlock(&pool->mutex);
			pthread_mutex_destroy(&data.mutex);
			assert(0);
			return 0;
		}

		/* wait for the thread to tell us it has gotten the data */
		pthread_mutex_lock(&data.mutex);
		pthread_cond_wait(&data.ready, &data.mutex);
		pthread_mutex_unlock(&data.mutex);
	}
	pool->idle_thread_count = pool->num_threads;
	pthread_mutex_unlock(&pool->mutex);
	pthread_mutex_destroy(&data.mutex);
	pthread_cond_destroy(&data.ready);
	return 1;
}

int fc_tpool_stop_threads (fc_tpool_t *pool)
{
	int i, rc;
	void *dummy;

	pthread_mutex_lock(&pool->mutex);
	pool->die = 1;
	pthread_mutex_unlock(&pool->mutex);

	for (i = 0; i < pool->num_threads; i++) {
		rc = pthread_join(pool->threads[i], &dummy);
		if (rc != 0) {
			return 0;
		}
	}
	return 1;
}

int fc_tpool_push_task (fc_tpool_t *pool,
		void (*callback) (void *input_data, void *output_data),
		void *input_data, void *output_data)
{
	fc_task_t task;

	task.id = pool->last_id + 1;
	task.callback = callback;
	task.input_data = input_data;
	task.output_data = output_data;
	if (!fc_fifo_push(&pool->taskq, &task)) {
		return 0;
	}

	pthread_mutex_lock(&pool->mutex);
	pool->last_id += 1;
	pthread_mutex_unlock(&pool->mutex);
	return task.id;
}

int fc_tpool_pop_result (fc_tpool_t *pool, void **ret)
{
	fc_task_t task;

	if (!fc_fifo_pop(&pool->resultq, &task)) {
		return 0;
	}

	*ret = task.output_data;
	return task.id;
}

void fc_tpool_clear_tasks (fc_tpool_t *pool)
{
	pthread_mutex_lock(&pool->mutex);
	fc_fifo_clear(&pool->taskq);
	memset(pool->discard_task, 0xff, pool->num_threads * sizeof(int));
	fc_fifo_clear(&pool->resultq);
	pthread_mutex_unlock(&pool->mutex);
}

#define FC_SYNCHRONIZED_RETURN(value, mutex_ptr) {\
	size_t _ret;\
	pthread_mutex_lock(mutex_ptr);\
	_ret = value;\
	pthread_mutex_unlock(mutex_ptr);\
	return _ret;\
}

size_t fc_tpool_size (fc_tpool_t *pool)
{
	FC_SYNCHRONIZED_RETURN(pool->num_threads, &pool->mutex);
}

size_t fc_tpool_num_idle_threads (fc_tpool_t *pool)
{
	FC_SYNCHRONIZED_RETURN(pool->idle_thread_count, &pool->mutex);
}

size_t fc_tpool_num_busy_threads (fc_tpool_t *pool)
{
	FC_SYNCHRONIZED_RETURN(pool->num_threads - pool->idle_thread_count,
			&pool->mutex);
}

size_t fc_tpool_num_pending_tasks (fc_tpool_t *pool)
{
	return fc_fifo_size(&pool->taskq);
}

size_t fc_tpool_num_pending_results (fc_tpool_t *pool)
{
	return fc_fifo_size(&pool->resultq);
}

