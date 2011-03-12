#ifndef _FC_MOVES_H_
#define _FC_MOVES_H_

#include <stdint.h>

typedef enum {
	FC_FIRST = 0,
	FC_SECOND,
	FC_THIRD,
	FC_FOURTH
} fc_player_t;

#define FC_NEXT_PLAYER(player) ((player + 1) % 4)
#define FC_PARTNER(player) ((player + 2) % 4)

typedef enum {
	FC_PAWN = 0,
	FC_BISHOP,
	FC_KNIGHT,
	FC_ROOK,
	FC_QUEEN,
	FC_KING
} fc_piece_t;

typedef struct {
	fc_player_t player;
	fc_piece_t piece;
	uint64_t move;
} fc_move_t;

typedef struct {
	fc_move_t *moves;
	int size;
	int index;
} fc_mlist_t;

uint64_t fc_uint64(const char *move);
void fc_move_copy (fc_move_t *dst, fc_move_t *src);
int fc_mlist_init (fc_mlist_t *list, int size);
int fc_mlist_resize (fc_mlist_t *list, int new_size);
int fc_mlist_append (fc_mlist_t *list,
		     fc_player_t player,
		     fc_piece_t piece,
		     uint64_t move);
int fc_mlist_copy (fc_mlist_t *dst, fc_mlist_t *src);
int fc_mlist_cat  (fc_mlist_t *dst, fc_mlist_t *src);
void fc_mlist_free (fc_mlist_t *list);
int fc_mlist_length (fc_mlist_t *list);
void fc_mlist_clear (fc_mlist_t *list);
fc_move_t *fc_mlist_get (fc_mlist_t *list, int index);

#endif
