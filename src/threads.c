/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 */

#include <pthread.h>
#include <stdlib.h>

#include "forchess/threads.h"

int fc_tpool_init (fc_tpool_t *pool, size_t num_threads)
{
	pool->num_threads = num_threads;
	pool->threads = calloc(num_threads, sizeof(pthread_t));
	if (!pool->threads) {
		return 0;
	}
	pthread_attr_init(&pool->attr);
	pthread_attr_setdetachstate(&pool->attr, PTHREAD_CREATE_JOINABLE);
	if (!fc_fifo_init(&pool->taskq, num_threads, sizeof(fc_task_t))) {
		free(pool->threads);
		return 0;
	}
	if (!fc_fifo_init(&pool->resultq, num_threads, sizeof(fc_task_t))) {
		free(pool->threads);
		fc_fifo_free(&pool->taskq);
		return 0;
	}
	pool->last_id = 0;
	pool->die = 0;
	return 1;
}

/*
 * WARNING: Only call this after the thread pool has been stopped.
 */
void fc_tpool_free (fc_tpool_t *pool)
{
	fc_fifo_free(&pool->taskq);
	fc_fifo_free(&pool->resultq);
	free(pool->threads);
}

static void *fc_thread_routine (void *data)
{
	fc_task_t task;
	fc_tpool_t *pool = data;

	/*
	 * Pull tasks from the queue and run them until we are told to die.
	 */
	while (1) {
		pthread_mutex_lock(&pool->mutex);
		if (pool->die) {
			pthread_mutex_unlock(&pool->mutex);
			break;
		}

		if (!fc_fifo_pop(&pool->taskq, &task)) {
			pthread_mutex_unlock(&pool->mutex);
			continue;
		}
		pthread_mutex_unlock(&pool->mutex);

		task.callback(task.input_data, task.output_data);

		/*
		 * Put the completed task onto the resultq.
		 */
		while (1) {
			pthread_mutex_lock(&pool->mutex);
			if (fc_fifo_push(&pool->resultq, &task)) {
				pthread_mutex_unlock(&pool->mutex);
				break;
			}
			pthread_mutex_unlock(&pool->mutex);
		}
	}

	pthread_exit(NULL);
}

int fc_tpool_start_threads (fc_tpool_t *pool)
{
	int i, rc;

	pthread_mutex_lock(&pool->mutex);
	pool->die = 0;
	for (i = 0; i < pool->num_threads; i++) {
		rc = pthread_create(&pool->threads[i], &pool->attr,
				&fc_thread_routine, pool);
		if (rc != 0) {
			/* tell previously created threads to stop */
			pool->die = 1;
			pthread_mutex_unlock(&pool->mutex);
			return 0;
		}
	}
	pthread_mutex_unlock(&pool->mutex);
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

/*
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
		pthread_mutex_unlock(&pool->mutex);
		return 0;
	}

	pthread_mutex_unlock(&pool->mutex);
	return task.id;
}
*/

