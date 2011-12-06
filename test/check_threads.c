#include <check.h>
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
}
END_TEST

Suite *thread_suite (void)
{
	Suite *s = suite_create("Threads");
	TCase *tc_threads = tcase_create("Core");
	tcase_add_test(tc_threads, test_thread_start_stop);
	tcase_add_test(tc_threads, test_thread_push);
	suite_add_tcase(s, tc_threads);
	return s;
}
