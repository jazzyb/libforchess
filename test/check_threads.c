#include <check.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "forchess/threads.h"

START_TEST (test_thread_start_stop)
{
	fc_tpool_t pool;
	fail_unless(fc_tpool_init(&pool, 4));
	fail_unless(fc_tpool_start_threads(&pool));
	fail_unless(fc_tpool_stop_threads(&pool));
	fc_tpool_free(&pool);
}
END_TEST

static void test_thread_push_callback (void *input, void *output)
{
	int *val = input;
	*val -= 1;
}

#define NUM_TEST_THREADS 16
START_TEST (test_thread_push)
{
	printf("    Running test_thread_push; this may take some time...");
	fflush(stdout);
	fc_tpool_t pool;
	fail_unless(fc_tpool_init(&pool, NUM_TEST_THREADS));
	fail_unless(fc_tpool_start_threads(&pool));
	int arr[NUM_TEST_THREADS];
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		arr[i] = i + 1;
		fail_unless(fc_tpool_push_task(&pool, test_thread_push_callback,
					arr + i, NULL));
	}
	sleep(1); /* give some time for threads to complete */
	fail_unless(fc_tpool_stop_threads(&pool));
	fc_tpool_free(&pool);
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		fail_unless(i == arr[i]);
	}
	printf("done.\n");
	fflush(stdout);
}
END_TEST

static void test_thread_pop_callback (void *input, void *output)
{
	int tmp, *in, *out;
	in = (int *)input;
	out = (int *)output;
	tmp = *in;
	*in = *out;
	*out = tmp;
	sleep(1);
}

#define NUM_POP_ITEMS (NUM_TEST_THREADS * 2)
START_TEST (test_thread_pop)
{
	printf("    Running test_thread_pop; this may take some more time...");
	fflush(stdout);
	int input[NUM_POP_ITEMS];
	int output[NUM_POP_ITEMS];
	fc_tpool_t pool;
	fail_unless(fc_tpool_init(&pool, NUM_TEST_THREADS));
	fail_unless(fc_tpool_start_threads(&pool));
	for (int i = 0; i < NUM_POP_ITEMS; i++) {
		input[i] = i;
		output[i] = NUM_POP_ITEMS - i;
		int id1 = fc_tpool_push_task(&pool,
				test_thread_pop_callback, input + i,
				output + i);
		if (id1) {
			fail_unless(id1 == i + 1);
			continue;
		}
		int *val;
		int id2 = fc_tpool_pop_result(&pool, (void **)&val);
		if (id2) {
			int id = id2 - 1;
			fail_unless(val == output + id);
			fail_unless(input[id] == NUM_POP_ITEMS - id);
			fail_unless(output[id] == id);
		}
	}
	sleep(2); /* let all remaining threads finish */
	int id;
	do {
		int *val;
		id = fc_tpool_pop_result(&pool, (void **)&val);
		if (id) {
			int i = id - 1;
			fail_unless(val == output + i);
			fail_unless(input[i] == NUM_POP_ITEMS - i);
			fail_unless(output[i] == i);
		}
	} while (id);
	pthread_mutex_lock(&pool.mutex);
	fail_unless(fc_fifo_is_empty(&pool.resultq));
	fail_unless(fc_fifo_is_empty(&pool.taskq));
	pthread_mutex_unlock(&pool.mutex);
	fail_unless(fc_tpool_stop_threads(&pool));
	fc_tpool_free(&pool);
	printf("done.\n");
	fflush(stdout);
}
END_TEST

Suite *thread_suite (void)
{
	Suite *s = suite_create("Threads");
	TCase *tc_threads = tcase_create("Core");
	tcase_add_test(tc_threads, test_thread_start_stop);
	tcase_add_test(tc_threads, test_thread_push);
	tcase_add_test(tc_threads, test_thread_pop);
	/* if the thread tests need some more time for whatever reason: */
	//tcase_set_timeout(tc_threads, 8);
	suite_add_tcase(s, tc_threads);
	return s;
}
