#include <check.h>
#include <stdio.h>
#include <stdlib.h>

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

Suite *thread_suite (void)
{
	Suite *s = suite_create("Threads");
	TCase *tc_threads = tcase_create("Core");
	tcase_add_test(tc_threads, test_thread_start_stop);
	suite_add_tcase(s, tc_threads);
	return s;
}
