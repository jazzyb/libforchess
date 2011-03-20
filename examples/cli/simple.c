#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "forchess/ai.h"
#include "forchess/board.h"
#include "forchess/moves.h"

int num_players (fc_board_t *board)
{
	int ret = 0;
	if (FC_BITBOARD((*board), FC_FIRST, FC_KING)) {
		ret += 1;
	}
	if (FC_BITBOARD((*board), FC_SECOND, FC_KING)) {
		ret += 1;
	}
	if (FC_BITBOARD((*board), FC_THIRD, FC_KING)) {
		ret += 1;
	}
	if (FC_BITBOARD((*board), FC_FOURTH, FC_KING)) {
		ret += 1;
	}
	return ret;
}

int validate_move (fc_board_t *board, fc_move_t *move, fc_player_t player)
{
	if (move->player != player) {
		return 0;
	}
	fc_mlist_t list;
	fc_mlist_init(&list, 0);
	fc_board_get_moves(board, &list, player);
	for (int i = 0; i < fc_mlist_length(&list); i++) {
		fc_move_t *other = fc_mlist_get(&list, i);
		if (move->piece == other->piece && move->move == other->move) {
			return fc_ai_is_move_valid(board, move);
		}
	}
	return 0;
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

	fc_board_t board;
	bzero(&board, sizeof(board));
	if (!fc_board_setup(&board, "examples/cli/simple.fc")) {
		fprintf(stderr, "error: cannot read start file\n");
		exit(1);
	}

	char piece;
	fc_move_t move;
	char move_buf[10]; /* FIXME: can overflow */
	for (fc_player_t player = FC_FIRST;; player = FC_NEXT_PLAYER(player)) {
		if (player_is_human[player]) {
			do {
				printf("%d: ", player + 1);
				fflush(stdout);
				gets(move_buf);
				fc_str2move(&board, &move, move_buf);
				if (!validate_move(&board, &move, player)) {
					fprintf(stderr, "error: invalid move\n");
				} else {
					break;
				}
			} while (1);
			if (!fc_board_make_move(&board, &move)) {
				/* pawn promotion */
retry:
				printf("new piece: ");
				fflush(stdout);
				scanf("%c", &piece);
				fc_piece_t type;
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
					goto retry;
				}
				if (!fc_board_make_pawn_move(&board, &move, type)) {
					fprintf(stderr, "error: unknown; quitting\n");
				}
			}
		} else { /* computer plays */
			int depth = num_players(&board) * 2;
			int ret = fc_ai_next_move(&board, &move, player, depth);
			if (!ret) {
				fprintf(stderr, "error: invalid AI move 1\n");
				exit(1);
			}
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
				fprintf(stderr, "error: invalid AI move 2\n");
				exit(1);
			}
			ret = fc_board_make_move(&board, &move);
			if (!ret) {
				/* FIXME: until we improve the API, just assume
				 * that we want a queen */
				ret = fc_board_make_pawn_move(&board, &move, FC_QUEEN);
				if (!ret) {
					fprintf(stderr, "error: invalid AI move 3\n");
					exit(1);
				}
			}

			fc_move2str(&board, move_buf, &move);
			int check_flag = fc_is_king_in_check(&board,
					FC_NEXT_PLAYER(player)) |
				fc_is_king_in_check(&board,
					FC_PARTNER(FC_NEXT_PLAYER(player)));
			if (check_flag == 1) {
				strcat(move_buf, "+");
			} else if (check_flag > 1) {
				strcat(move_buf, "++");
			}
			printf("%d: %c%s\n", player + 1, piece, move_buf);
			fflush(stdout);
		}
	}

	return 0;
}
