#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "forchess/moves.h"

START_TEST (test_move_copy)
{
	fc_move_t dst, src;
	src.player = FC_QUEEN;
	src.piece = FC_THIRD;
	src.promote = FC_KNIGHT;
	src.move = 129;
	fc_move_copy(&dst, &src);
	fail_unless(dst.player == src.player &&
		    dst.piece == src.piece &&
		    dst.promote == src.promote &&
		    dst.move == src.move);
}
END_TEST

START_TEST (test_mlist_init)
{
	fc_mlist_t l1, l2;
	int ret = fc_mlist_init(&l1, 10);
	fail_unless(ret == 1 && l1.moves && l1.index == 0 && l1.size == 10);
	ret = fc_mlist_init(&l2, 0);
	fail_unless(ret == 1 && l2.moves && l2.index == 0 &&
		    l2.size == 130);
}
END_TEST

START_TEST (test_mlist_append)
{
	fc_mlist_t list;
	int ret = fc_mlist_init(&list, 0);
	fail_unless(ret == 1);
	ret = fc_mlist_append(&list, FC_SECOND, FC_ROOK, FC_NONE, 129);
	fail_unless(ret == 1 &&
		    list.moves[0].player == FC_SECOND &&
		    list.moves[0].piece == FC_ROOK &&
		    list.moves[0].move == 129);
}
END_TEST

START_TEST (test_mlist_resize)
{
	fc_mlist_t list;
	int ret = fc_mlist_init(&list, 20);
	fail_unless(ret == 1);
	ret = fc_mlist_resize(&list, 25);
	fail_unless(ret == 1 && list.size == 25);

	fc_mlist_t small;
	ret = fc_mlist_init(&small, 2);
	fail_unless(ret == 1 && small.size == 2);
	ret = fc_mlist_append(&small, FC_SECOND, FC_ROOK, FC_NONE, 129);
	fail_unless(ret == 1);
	ret = fc_mlist_append(&small, FC_THIRD, FC_QUEEN, FC_NONE, 33);
	fail_unless(ret == 1);
	ret = fc_mlist_append(&small, FC_FOURTH, FC_PAWN, FC_NONE, 17);
	fail_unless(ret == 1 && small.size > 2);
}
END_TEST

START_TEST (test_mlist_copy)
{
	fc_mlist_t small, big;
	int ret = fc_mlist_init(&small, 2);
	ret = fc_mlist_init(&big, 10);
	ret = fc_mlist_append(&big, FC_SECOND, FC_ROOK, FC_NONE, 129);
	ret = fc_mlist_append(&big, FC_THIRD, FC_QUEEN, FC_NONE, 33);
	ret = fc_mlist_append(&big, FC_FOURTH, FC_PAWN, FC_NONE, 17);
	ret = fc_mlist_append(&big, FC_THIRD, FC_KING, FC_NONE, 33);
	ret = fc_mlist_append(&big, FC_FOURTH, FC_BISHOP, FC_NONE, 17);
	ret = fc_mlist_copy(&small, &big);
	fail_unless(ret == 1 &&
		    small.size == big.size &&
		    small.index == big.index &&
		    small.moves[small.index-1].piece == FC_BISHOP);
}
END_TEST

START_TEST (test_mlist_cat)
{
	fc_mlist_t small, big;
	int ret = fc_mlist_init(&small, 2);
	ret = fc_mlist_append(&small, FC_FIRST, FC_KNIGHT, FC_NONE, 3);
	ret = fc_mlist_append(&small, FC_FIRST, FC_ROOK, FC_NONE, 129);
	ret = fc_mlist_init(&big, 10);
	ret = fc_mlist_append(&big, FC_SECOND, FC_ROOK, FC_NONE, 129);
	ret = fc_mlist_append(&big, FC_THIRD, FC_QUEEN, FC_NONE, 33);
	ret = fc_mlist_append(&big, FC_FOURTH, FC_PAWN, FC_NONE, 17);
	ret = fc_mlist_append(&big, FC_THIRD, FC_KING, FC_NONE, 33);
	ret = fc_mlist_append(&big, FC_FOURTH, FC_BISHOP, FC_NONE, 17);
	ret = fc_mlist_cat(&small, &big);
	fail_unless(ret == 1 &&
		    small.index == 7 &&
		    small.moves[6].piece == FC_BISHOP);
}
END_TEST

Suite *move_suite (void)
{
	Suite *s = suite_create("Moves");
	TCase *tc_moves = tcase_create("Core");
	tcase_add_test(tc_moves, test_move_copy);
	tcase_add_test(tc_moves, test_mlist_init);
	tcase_add_test(tc_moves, test_mlist_append);
	tcase_add_test(tc_moves, test_mlist_resize);
	tcase_add_test(tc_moves, test_mlist_copy);
	tcase_add_test(tc_moves, test_mlist_cat);
	suite_add_tcase(s, tc_moves);
	return s;
}
