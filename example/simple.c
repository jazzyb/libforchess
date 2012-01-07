/* FIXME The -j option doesn't work now.  Remove all threading code form this
 * file. */
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <forchess/ai.h>
#include <forchess/board.h>
#include "game.h"

void query_human_for_move (fc_game_t *game, fc_player_t player);
void get_move (fc_game_t *game, fc_move_t *move, fc_player_t player);
void str2move (fc_game_t *game, fc_move_t *move, char *str);
fc_piece_t get_pawn_promotion (void);
void make_computer_move (fc_game_t *game, fc_player_t player, int depth,
		int timeout);
void get_time (char *str, time_t t);
void move2str (fc_game_t *game, char *str, fc_move_t *move);

int get_arguments (int argc, char **argv, char *file, int *depth, int *timeout)
{
	int c;
	*depth = *timeout = 0;
	while ((c = getopt(argc, argv, "d:f:ht:")) != -1) {
		switch (c) {
		case 'd':
			*depth = strtol(optarg, NULL, 10);
			break;
		case 'h':
			fprintf(stderr,
				"%s {[-h] | [-d <depth>] [-f <filename>] [-t <seconds>] [1] [2] [3] [4]}\n"
				"    Play forchess!\n"
				"\n"
				"    -h: show this help message\n"
				"\n"
				"    usage: list the players on the command line who\n"
				"           will be played by humans; the remainder\n"
				"           will be played by the AI\n"
				"    options:\n"
				"           -d: the number of turns that you wish the\n"
				"               program to search; by default the program\n"
				"               will search 2 turns ahead\n"
				"           -f: game file to load; if none given then a\n"
				"               new game will be started\n"
				"           -t: the maximum number of seconds the program\n"
				"               should spend searching for a move\n"
				"    for example: '%s 1 3' will start a game in\n"
				"           which players 2 and 4 will be played by\n"
				"           the computer\n\n", argv[0], argv[0]);
			exit(0);
		case 'f':
			strcpy(file, optarg);
			break;
		case 't':
			*timeout = strtol(optarg, NULL, 10);
			break;
		case '?':
			if (optopt == 'd' || optopt == 't') {
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			} else {
				fprintf(stderr, "Unknown option: -%c\n", optopt);
			}
		default:
			abort();
		}
	}
	if (argc - optind > 5) {
		fprintf(stderr, "error: too many arguments; see '%s -h'\n", argv[0]);
		exit(1);
	}
	return optind;
}

int main (int argc, char **argv)
{
	int depth, timeout;
	char filename[256] = "example/simple.fc";
	int optind = get_arguments(argc, argv, filename, &depth, &timeout);

	int player_is_human[] = {0, 0, 0, 0};
	for (int i = optind; i < argc; i++) {
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
	if (!fc_game_load(&game, filename)) {
		fprintf(stderr, "error: cannot read file '%s'\n", filename);
		exit(1);
	}

	for (fc_player_t player = fc_game_current_player(&game);
	     !fc_game_is_over(&game);
	     player = fc_game_next_player(&game)) {
		if (player_is_human[player]) {
			query_human_for_move(&game, player);
		} else {
			make_computer_move(&game, player, depth, timeout);
		}
	}

	return 0;
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

void get_move (fc_game_t *game, fc_move_t *move, fc_player_t player)
{
	char move_buf[100]; /* FIXME can overflow */
	do {
		printf("%d: ", player + 1);
		fflush(stdout);
		(void)gets(move_buf);
		str2move(game, move, move_buf);
		if (!fc_game_is_move_legal(game, move)) {
			fprintf(stderr, "error: invalid move\n");
		} else {
			return;
		}
	} while (1);
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

/*
 * Called by make_computer_move below.
 */
void get_best_move (fc_game_t *game, fc_move_t *move, fc_player_t player,
		int depth, int timeout)
{
	int rc;
	fc_mlist_t *tmp = NULL;
	fc_ai_t ai;
	if (depth > fc_game_number_of_players(game) * 2) {
		tmp = calloc(1, sizeof(fc_mlist_t));
		fc_mlist_init(tmp);
		fc_ai_init(&ai, fc_game_get_board(game));
		rc = fc_ai_next_ranked_moves(&ai, tmp, NULL, player,
				fc_game_number_of_players(game) * 2, 0);
		assert(rc);
	}
	fc_ai_init(&ai, fc_game_get_board(game));
	rc = fc_ai_next_move(&ai, move, tmp, player, depth, timeout);
	assert(rc);
	if (tmp) {
		fc_mlist_free(tmp);
		free(tmp);
	}
}

void make_computer_move (fc_game_t *game, fc_player_t player, int depth,
		int timeout)
{
	fc_move_t move;
	if (!depth) {
		depth = fc_game_number_of_players(game) * 2;
	} else {
		depth = fc_game_number_of_players(game) * depth;
	}

	time_t start = time(NULL);
	get_best_move(game, &move, player, depth, timeout);
	char time_str[100];
	get_time(time_str, time(NULL) - start);

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

	int check_status = fc_game_opponent_kings_check_status(game, player,
			&move);
	if (check_status == FC_CHECK) {
		strcat(move_buf, "+ ");
	} else if (check_status == FC_CHECKMATE) {
		strcat(move_buf, "++");
	} else {
		strcat(move_buf, "  ");
	}

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

	if (!fc_game_make_move(game, &move)) {
		assert(0);
	}
	printf("%d: %c%s :: %s\n", player + 1, piece, move_buf, time_str);
	fflush(stdout);
}

void get_time (char *str, time_t t)
{
	int mins = t / 60;
	int secs = t - (mins * 60);
	if (mins) {
		sprintf(str, "%dm%02ds", mins, secs);
	} else {
		sprintf(str, "%ds", secs);
	}
}

void move2str (fc_game_t *game, char *str, fc_move_t *move)
{
	int x1, y1, x2, y2;
	fc_game_convert_move_to_coords(game, &x1, &y1, &x2, &y2, move);
	sprintf(str, "%c%c-%c%c", (char)x1 + 'a', (char)y1 + '1',
			(char)x2 + 'a', (char)y2 + '1');
}
