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
	/* TODO add the other values of the move struct */
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

START_TEST (test_mlist_insert1)
{
	fc_move_t move;
	fc_mlist_t list;
	int ret = fc_mlist_init(&list, 0);
	fail_unless(ret == 1);
	move.player = FC_SECOND;
	move.piece = FC_ROOK;
	move.opp_player = FC_THIRD;
	move.opp_piece = FC_QUEEN;
	move.promote = FC_NONE;
	move.move = 129;
	ret = fc_mlist_insert(&list, &move, 0);
	fail_unless(ret == 1 &&
		    list.moves[0].player == FC_SECOND &&
		    list.moves[0].piece == FC_ROOK &&
		    list.moves[0].opp_player == FC_THIRD &&
		    list.moves[0].opp_piece == FC_QUEEN &&
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
	fc_move_t move;
	move.player = FC_SECOND;
	move.piece = FC_ROOK;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 129;
	ret = fc_mlist_insert(&small, &move, 0);
	fail_unless(ret == 1);
	move.player = FC_THIRD;
	move.piece = FC_QUEEN;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 33;
	ret = fc_mlist_insert(&small, &move, 0);
	fail_unless(ret == 1);
	move.player = FC_FOURTH;
	move.piece = FC_PAWN;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 17;
	ret = fc_mlist_insert(&small, &move, 0);
	fail_unless(ret == 1 && small.size > 2);
}
END_TEST

START_TEST (test_mlist_copy)
{
	fc_mlist_t small, big;
	int ret = fc_mlist_init(&small, 2);
	ret = fc_mlist_init(&big, 10);
	fc_move_t move;
	move.player = FC_SECOND;
	move.piece = FC_ROOK;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 129;
	ret = fc_mlist_insert(&big, &move, 0);
	move.player = FC_THIRD;
	move.piece = FC_QUEEN;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 33;
	ret = fc_mlist_insert(&big, &move, 0);
	move.player = FC_FOURTH;
	move.piece = FC_PAWN;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 17;
	ret = fc_mlist_insert(&big, &move, 0);
	move.player = FC_THIRD;
	move.piece = FC_KING;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 33;
	ret = fc_mlist_insert(&big, &move, 0);
	move.player = FC_FOURTH;
	move.piece = FC_BISHOP;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 17;
	ret = fc_mlist_insert(&big, &move, 0);
	ret = fc_mlist_copy(&small, &big);
	fail_unless(ret == 1 &&
		    small.size == big.size &&
		    small.index == big.index &&
		    small.moves[small.index-1].piece == FC_BISHOP);
}
END_TEST

START_TEST (test_mlist_merge)
{
	fc_mlist_t small, big;
	int ret = fc_mlist_init(&small, 2);
	fc_move_t move;
	move.player = FC_FIRST;
	move.piece = FC_KNIGHT;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 3;
	ret = fc_mlist_insert(&small, &move, 20);
	move.player = FC_FIRST;
	move.piece = FC_ROOK;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 129;
	ret = fc_mlist_insert(&small, &move, 19);
	ret = fc_mlist_init(&big, 10);
	move.player = FC_SECOND;
	move.piece = FC_ROOK;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 129;
	ret = fc_mlist_insert(&big, &move, 18);
	move.player = FC_THIRD;
	move.piece = FC_QUEEN;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 33;
	ret = fc_mlist_insert(&big, &move, 17);
	move.player = FC_FOURTH;
	move.piece = FC_PAWN;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 17;
	ret = fc_mlist_insert(&big, &move, 16);
	move.player = FC_THIRD;
	move.piece = FC_KING;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 33;
	ret = fc_mlist_insert(&big, &move, 15);
	move.player = FC_FOURTH;
	move.piece = FC_BISHOP;
	move.opp_player = FC_NONE;
	move.opp_piece = FC_NONE;
	move.promote = FC_NONE;
	move.move = 17;
	ret = fc_mlist_insert(&big, &move, 14);
	ret = fc_mlist_merge(&small, &big);
	fail_unless(ret == 1 &&
		    small.index == 7 &&
		    small.moves[6].piece == FC_BISHOP);
}
END_TEST

START_TEST (test_mlist_insert2)
{
	fc_move_t move;
	fc_mlist_t list;
	int ret = fc_mlist_init(&list, 0);
	fail_unless(ret == 1);
	move.value = 100;
	ret = fc_mlist_insert(&list, &move, move.value);
	fail_unless(ret == 1);
	move.value = 10;
	ret = fc_mlist_insert(&list, &move, move.value);
	fail_unless(ret == 1);
	move.value = 40;
	ret = fc_mlist_insert(&list, &move, move.value);
	fail_unless(ret == 1);
	move.value = 60;
	ret = fc_mlist_insert(&list, &move, move.value);
	fail_unless(ret == 1);
	move.value = 200;
	ret = fc_mlist_insert(&list, &move, move.value);
	fail_unless(ret == 1);
	move.value = 40;
	ret = fc_mlist_insert(&list, &move, move.value);
	fail_unless(ret == 1);
	fail_unless(fc_mlist_get(&list, 0)->value == 200);
	fail_unless(fc_mlist_get(&list, 1)->value == 100);
	fail_unless(fc_mlist_get(&list, 2)->value == 60);
	fail_unless(fc_mlist_get(&list, 3)->value == 40);
	fail_unless(fc_mlist_get(&list, 4)->value == 40);
	fail_unless(fc_mlist_get(&list, 5)->value == 10);
}
END_TEST

Suite *move_suite (void)
{
	Suite *s = suite_create("Moves");
	TCase *tc_moves = tcase_create("Core");
	tcase_add_test(tc_moves, test_move_copy);
	tcase_add_test(tc_moves, test_mlist_init);
	tcase_add_test(tc_moves, test_mlist_insert1);
	tcase_add_test(tc_moves, test_mlist_resize);
	tcase_add_test(tc_moves, test_mlist_copy);
	tcase_add_test(tc_moves, test_mlist_merge);
	tcase_add_test(tc_moves, test_mlist_insert2);
	suite_add_tcase(s, tc_moves);
	return s;
}
