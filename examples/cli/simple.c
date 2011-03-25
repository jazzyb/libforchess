#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "forchess/ai.h"
#include "forchess/board.h"
#include "forchess/game.h"

void move2str (fc_game_t *game, char *str, fc_move_t *move)
{
	int x1, y1, x2, y2;
	fc_game_convert_move_to_coords(game, &x1, &y1, &x2, &y2, move);
	sprintf(str, "%c%c-%c%c", (char)x1 + 'a', (char)y1 + '1',
			(char)x2 + 'a', (char)y2 + '1');
}

void str2move (fc_game_t *game, fc_move_t *move, char *str)
{
	char x1, y1, x2, y2;
	int ret = sscanf(str, "%c%c-%c%c", &x1, &y1, &x2, &y2);
	x1 -= 'a';
	y1 -= '1';
	if (ret > 2) {
		x2 -= 'a';
		y2 -= '1';
	}
	fc_game_convert_coords_to_move(game, move, (int)x1, (int)y1,
			(int)x2, (int)y2);
}

void get_move (fc_game_t *game, fc_move_t *move, fc_player_t player)
{
	char move_buf[100]; /* FIXME can overflow */
	do {
		printf("%d: ", player + 1);
		fflush(stdout);
		(void)gets(move_buf);
		str2move(game, move, move_buf);
		if (!fc_game_is_move_valid(game, move)) {
			fprintf(stderr, "error: invalid move\n");
		} else {
			return;
		}
	} while (1);
}

fc_piece_t get_pawn_promotion (void)
{
	char piece;
	fc_piece_t type = FC_NONE;
	do {
		printf("new piece: ");
		fflush(stdout);
		(void)scanf("%c", &piece);
		switch (piece) {
		case 'B':
			type = FC_BISHOP; break;
		case 'N':
			type = FC_KNIGHT; break;
		case 'R':
			type = FC_ROOK; break;
		case 'Q':
			type = FC_QUEEN; break;
		default:
			fprintf(stderr, "error: invalid promotion\n");
			continue;
		}
	} while (0);
}

void query_human_for_move (fc_game_t *game, fc_player_t player)
{
	fc_move_t move;
	get_move(game, &move, player);
	if (!fc_game_make_move(game, &move)) {
		/* pawn promotion */
		fc_game_set_promote_pawn(&move, get_pawn_promotion());
		if (!fc_game_make_move(game, &move)) {
			assert(0);
		}
	}
}

void make_computer_move (fc_game_t *game, fc_player_t player)
{
	fc_move_t move;
	int depth = fc_game_number_of_players(game) * 2;
	if (!fc_ai_next_move(game->board, &move, player, depth)) {
		assert(0);
	}

	char piece;
	switch (move.piece) {
	case FC_PAWN:
		piece = ' '; break;
	case FC_BISHOP:
		piece = 'B'; break;
	case FC_KNIGHT:
		piece = 'N'; break;
	case FC_ROOK:
		piece = 'R'; break;
	case FC_QUEEN:
		piece = 'Q'; break;
	case FC_KING:
		piece = 'K'; break;
	default:
		assert(0);
	}

	char move_buf[100];
	move2str(game, move_buf, &move);
	char promote = '\0';
	switch (move.promote) {
	case FC_BISHOP:
		promote = 'B'; break;
	case FC_KNIGHT:
		promote = 'N'; break;
	case FC_ROOK:
		promote = 'R'; break;
	case FC_QUEEN:
		promote = 'Q'; break;
	}
	if (promote != '\0') {
		char promotion[10];
		sprintf(promotion, " (%c)", promote);
		strcat(move_buf, promotion);
	}
	int check_status = fc_game_opponent_kings_check_status(game, player,
			&move);
	if (check_status == FC_CHECK) {
		strcat(move_buf, "+");
	} else if (check_status == FC_CHECKMATE) {
		strcat(move_buf, "++");
	}

	if (!fc_game_make_move(game, &move)) {
		assert(0);
	}
	/* TODO create an API call which will determine whether
	 * or not a move will put a king in check(mate) */
	/*
	int check_flag_before = fc_game_opponent_kings_check_status(game,
			player);
	if (!fc_game_make_move(game, &move)) {
		assert(0);
	}

	int check_flag_after = fc_game_opponent_kings_check_status(game,
				player);
	if (check_flag_after == 1 && check_flag_after != check_flag_before) {
		strcat(move_buf, "+");
	} else if (check_flag_after > 1 &&
			check_flag_after != check_flag_before) {
		strcat(move_buf, "++");
	}
	*/
	printf("%d: %c%s\n", player + 1, piece, move_buf);
	fflush(stdout);
}

int main (int argc, char **argv)
{
	if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		fprintf(stderr,
		        "simple {[-h] | [1] [2] [3] [4]}\n"
		        "    Play forchess!\n"
		        "\n"
		        "    -h: show this help message\n"
		        "\n"
		        "    usage: list the players on the command line who\n"
		        "           will be played by humans; the remainder\n"
		        "           will be played by the AI\n"
		        "    for example: 'simple 1 3' will start a game in\n"
		        "           which players 2 and 4 will be played by\n"
		        "           the computer\n\n");
		exit(0);
	} else if (argc > 5) {
		fprintf(stderr, "error: too many arguments; see 'simple -h'\n");
		exit(1);
	}

	int player_is_human[] = {0, 0, 0, 0};
	for (int i = 1; i < argc; i++) {
		int n = strtol(argv[i], NULL, 10);
		if (n > 0 && n <= 4) {
			player_is_human[n-1] = 1;
		} else {
			fprintf(stderr, "error: argument %d, '%s' is not a"
					"valid player\n", n, argv[i]);
			exit(1);
		}
	}

	fc_game_t game;
	fc_game_init(&game);
	/* FIXME the two lines below should be combined into fc_game_load() */
	game.player = FC_FIRST;
	if (!fc_board_setup(game.board, "examples/cli/simple.fc")) {
		fprintf(stderr, "error: cannot read start file\n");
		exit(1);
	}

	for (fc_player_t player = fc_game_current_player(&game);
	     !fc_game_is_over(&game);
	     player = fc_game_next_player(&game)) {
		if (player_is_human[player]) {
			query_human_for_move(&game, player);
		} else {
			make_computer_move(&game, player);
		}
	}

	return 0;
}
