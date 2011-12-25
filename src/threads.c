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
	int ready;
	pthread_mutex_t mutex;
};

static void *fc_thread_routine (void *data)
{
	int id;
	fc_task_t task;
	fc_tpool_t *pool;
	struct thread_data *tdp = (struct thread_data *)data;

	pthread_mutex_lock(&tdp->mutex);
	id = tdp->thread_id;
	pool = tdp->pool;
	tdp->ready = 1;
	pthread_mutex_unlock(&tdp->mutex);

	/* pull tasks from the queue and run them until we are told to die */
	while (1) {
		pthread_mutex_lock(&pool->mutex);
		if (pool->die) {
			pthread_mutex_unlock(&pool->mutex);
			goto exit;
		}

		if (pool->discard_task[id]) {
			pool->discard_task[id] = 0;
		}

		if (!fc_fifo_pop(&pool->taskq, &task)) {
			pthread_mutex_unlock(&pool->mutex);
			continue;
		}

		assert(pool->idle_thread_count > 0);
		pool->idle_thread_count -= 1;
		pthread_mutex_unlock(&pool->mutex);

		task.callback(task.input_data, task.output_data);

		/* put the completed task onto the resultq */
		while (1) {
			pthread_mutex_lock(&pool->mutex);
			if (pool->die) {
				pthread_mutex_unlock(&pool->mutex);
				goto exit;
			}

			if (pool->discard_task[id]) {
				pool->discard_task[id] = 0;
				break;
			}

			if (fc_fifo_push(&pool->resultq, &task)) {
				break;
			}
			pthread_mutex_unlock(&pool->mutex);
		}

		assert(pool->idle_thread_count != pool->num_threads);
		pool->idle_thread_count += 1;
		pthread_mutex_unlock(&pool->mutex);
	}

exit:
	pthread_exit(NULL);
}

int fc_tpool_start_threads (fc_tpool_t *pool)
{
	int i, rc;
	struct thread_data data;

	pthread_mutex_init(&data.mutex, NULL);
	pthread_mutex_lock(&pool->mutex);
	pool->die = 0;
	data.pool = pool;
	for (i = 0; i < pool->num_threads; i++) {
		data.thread_id = i;
		data.ready = 0;
		rc = pthread_create(&pool->threads[i], &pool->attr,
				&fc_thread_routine, &data);
		if (rc != 0) {
			/* tell previously created threads to stop */
			pool->die = 1;
			pthread_mutex_unlock(&pool->mutex);
			pthread_mutex_destroy(&data.mutex);
			return 0;
		}

		/* wait for the thread to tell us it has gotten the data */
		while (1) {
			pthread_mutex_lock(&data.mutex);
			if (data.ready) {
				pthread_mutex_unlock(&data.mutex);
				break;
			}
			pthread_mutex_unlock(&data.mutex);
		}
	}
	pool->idle_thread_count = pool->num_threads;
	pthread_mutex_unlock(&pool->mutex);
	pthread_mutex_destroy(&data.mutex);
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

int fc_tpool_kill_threads (fc_tpool_t *pool)
{
	int i, rc;

	for (i = 0; i < pool->num_threads; i++) {
		rc = pthread_cancel(pool->threads[i]);
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

	pthread_mutex_lock(&pool->mutex);
	if (fc_fifo_is_full(&pool->taskq)) {
		pthread_mutex_unlock(&pool->mutex);
		return 0;
	}

	task.id = ++pool->last_id;
	task.callback = callback;
	task.input_data = input_data;
	task.output_data = output_data;
	if (!fc_fifo_push(&pool->taskq, &task)) {
		/*
		 * We should have been able to push since we were told the
		 * queue was not full.  This indicates a threading issue.
		 */
		pthread_mutex_unlock(&pool->mutex);
		assert(0);
		return 0;
	}

	pthread_mutex_unlock(&pool->mutex);
	return task.id;
}

int fc_tpool_pop_result (fc_tpool_t *pool, void **ret)
{
	fc_task_t task;

	pthread_mutex_lock(&pool->mutex);
	if (!fc_fifo_pop(&pool->resultq, &task)) {
		pthread_mutex_unlock(&pool->mutex);
		return 0;
	}
	pthread_mutex_unlock(&pool->mutex);

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
	FC_SYNCHRONIZED_RETURN(fc_fifo_size(&pool->taskq), &pool->mutex);
}

size_t fc_tpool_num_pending_results (fc_tpool_t *pool)
{
	FC_SYNCHRONIZED_RETURN(fc_fifo_size(&pool->resultq), &pool->mutex);
}

