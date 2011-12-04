#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include "forchess/fifo.h"

START_TEST (test_push)
{
	fc_fifo_t queue;
	int ret = fc_fifo_init(&queue, 4, sizeof(int));
	fail_unless(ret == 1 && fc_fifo_is_empty(&queue));
	int val = 1;
	ret = fc_fifo_push(&queue, &val);
	fail_unless(ret == 1);
	ret = fc_fifo_push(&queue, &val);
	fail_unless(ret == 1);
	ret = fc_fifo_push(&queue, &val);
	fail_unless(ret == 1);
	ret = fc_fifo_push(&queue, &val);
	fail_unless(ret == 1);
	ret = fc_fifo_push(&queue, &val);
	fail_unless(ret == 0 && fc_fifo_is_full(&queue));
	fc_fifo_free(&queue);
}
END_TEST

START_TEST (test_pop)
{
	int result;
	fc_fifo_t queue;
	int ret = fc_fifo_init(&queue, 4, sizeof(int));
	fail_unless(ret == 1 && fc_fifo_pop(&queue, &result) == 0);
	for (int i = 0; i < 4; i++) {
		fail_unless(fc_fifo_push(&queue, &i) == 1);
	}
	for (int i = 0; i < 8; i++) {
		ret = fc_fifo_pop(&queue, &result);
		fail_unless(ret == 1 && i == result);
		if (i < 4) {
			int j = i + 4;
			fail_unless(fc_fifo_push(&queue, &j) == 1);
		}
	}
	fail_unless(fc_fifo_is_empty(&queue));
}
END_TEST

Suite *fifo_suite (void)
{
	Suite *s = suite_create("FIFO");
	TCase *tc_fifo = tcase_create("Core");
	tcase_add_test(tc_fifo, test_push);
	tcase_add_test(tc_fifo, test_pop);
	suite_add_tcase(s, tc_fifo);
	return s;
}
