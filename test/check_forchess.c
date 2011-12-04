#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

extern Suite *move_suite (void);
extern Suite *board_suite (void);
extern Suite *check_suite (void);
extern Suite *fifo_suite (void);
extern Suite *thread_suite (void);
extern Suite *ai_suite (void);
extern Suite *game_suite (void);

int main (int argc, char **argv)
{
	int number_failed;
	SRunner *sr = srunner_create(move_suite());
	srunner_add_suite(sr, board_suite());
	srunner_add_suite(sr, check_suite());
	srunner_add_suite(sr, fifo_suite());
	srunner_add_suite(sr, thread_suite());
	srunner_add_suite(sr, ai_suite());
	srunner_add_suite(sr, game_suite());
	/* uncomment the below if we need to run gdb */
	//srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}
