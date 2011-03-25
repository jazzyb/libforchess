#ifndef _FORCHESS_GAME_H_
#define _FORCHESS_GAME_H_
/*
 * X fc_game_init(fc_game_t *);
 * fc_game_save(fc_game_t *, const char *);
 * fc_game_load(fc_game_t *, const char *);
 * X fc_game_is_over(fc_game_t *);
 * X fc_game_current_player(fc_game_t *);
 * X fc_game_next_player(fc_game_t *);
 * fc_game_convert_coords_to_move(fc_game_t *, fc_move_t *, int, int, int, int);
 * X fc_game_convert_move_to_coords(fc_game_t *, int*, int*, int*, int*, fc_move_t *);
 * X fc_game_promote_pawn(fc_move_t *, fc_piece_t);
 * X fc_game_is_move_valid(fc_game_t *, fc_move_t *);
 * X fc_game_make_move(fc_game_t *, fc_move_t *);
 * X fc_game_number_players(fc_game_t *);
 * X fc_game_is_king_in_check(fc_game_t *, fc_player_t);
 */
#include "forchess/move.h"
#include "forchess/board.h"

typedef struct {
	fc_player_t player;
	fc_board_t *board;
} fc_game_t;

int fc_game_init (fc_game_t *game);
void fc_game_free (fc_game_t *game);
fc_player_t fc_game_current_player (fc_game_t *game);
fc_player_t fc_game_next_player (fc_game_t *game);
int fc_game_number_of_players (fc_game_t *game);
int fc_game_king_check_status (fc_game_t *game, fc_player_t player);
int fc_game_is_move_valid (fc_game_t *game, fc_move_t *move);
int fc_game_is_over (fc_game_t *game);
int fc_game_make_move (fc_game_t *game, fc_move_t *move);
void fc_game_set_promote_pawn (fc_move_t *move, fc_piece_t promote);
int fc_game_convert_move_to_coords (fc_game_t *game, int *x1, int *y1,
		int *x2, int *y2, fc_move_t *move);
int fc_game_convert_coords_to_move (fc_game_t *game, fc_move_t *move,
		int x1, int y1, int x2, int y2);

#endif
