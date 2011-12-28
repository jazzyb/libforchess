/*
 * LibForchess
 * Copyright (c) 2011, Jason M Barnes
 *
 * This file is subject to the terms and conditions of the 'LICENSE' file
 * which is a part of this source code package.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FC_GAME_H_
#define _FC_GAME_H_

/**
 * @author jazzyb aka Jason M Barnes
 * @file forchess/game.h
 * @brief API for the high-level manipulation of a forchess game.
 *
 * This API may be ignored by any users of LibForchess.  It was only written to
 * ease the development of games with the forchess AI because the internal
 * structure of the LibForchess API (e.g. bitboards and other structs) are
 * optimized for speed and not necessarily ease of use.
 *
 * However, the high-level API will continue to be updated and improved with
 * the rest of the code, so the developer should feel comfortable using it if
 * he/she so desires.
 */

#ifndef DOXYGEN_IGNORE

#include "forchess/board.h"
#include "forchess/moves.h"

typedef struct {
	fc_player_t player;
	fc_board_t *board;
} fc_game_t;

#endif /* DOXYGEN_IGNORE */

/**
 * @brief Initialize the game structure.
 *
 * Space should be allocated for the game struct before the call to this
 * function.
 *
 * @param[out] game A pointer to the game.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_game_init (fc_game_t *game);

/**
 * @brief De-initializes the game structure.
 *
 * Any data-structures internal to the game struct are freed, but if the user
 * allocated space for the game struct itself, then they must free it
 * separately.
 *
 * @param[out] game A pointer to the game.
 *
 * @return void
 */
void fc_game_free (fc_game_t *game);

/**
 * @brief Saves a game to filename.
 *
 * @param[in] game A pointer to the game.
 * @param[in] filename The name of the file to save the game to.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_game_save (fc_game_t *game, const char *filename);

/**
 * @brief Load a previously saved game.
 *
 * @param[out] game A pointer to the game.
 * @param[in] filename The name of the file to read the saved game from.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_game_load (fc_game_t *game, const char *filename);

/**
 * @brief Return the current player.
 *
 * @param[in] game A pointer to the game.
 *
 * @return current player
 */
fc_player_t fc_game_current_player (fc_game_t *game);

/**
 * @brief Return the game's board.
 *
 * @param[in] game A pointer to the game.
 *
 * @return a pointer to the game board
 */
fc_board_t *fc_game_get_board (fc_game_t *game);

/**
 * @brief Update to the next player in the game.
 *
 * E.g. If the current player if FC_SECOND and FC_THIRD is out of the game,
 * then the new current player becomes FC_FOURTH.
 *
 * @param[in,out] game A pointer to the game.
 *
 * @return the next (new current) player in the game
 */
fc_player_t fc_game_next_player (fc_game_t *game);

/**
 * @brief Return the number of players currently in the game.
 *
 * @param[in] game A pointer to the game.
 *
 * @return the number of players currently in the game; [1, 4] inclusive
 */
int fc_game_number_of_players (fc_game_t *game);

/**
 * @brief Return the check status of the player's king.
 *
 * @remark Simply calls fc_board_check_status(), so please see its
 * documentation for details.
 *
 * @param[in] game A pointer to the game.
 * @param[in] player The player whose king we want the status of.
 *
 * @return 0 if the player's king is not in check; FC_CHECK if the king is in
 * check; FC_CHECKMATE if the king is in checkmate
 */
int fc_game_king_check_status (fc_game_t *game, fc_player_t player);

/**
 * @brief Returns the check status of player's opponents if the move is made.
 *
 * @note Does not actually apply the move to the game.
 *
 * @param[in] game A pointer to the game.
 * @param[in] player The player whose opponents' kings we want to know the
 * status for.
 * @param[in] move The move which is to be made.
 *
 * @return 0 if the move does not put the opponent kings in check; FC_CHECK if
 * the move puts at least one opponent's king into check who was not previously
 * in check; FC_CHECKMATE if the move puts at least one opponent's king into
 * checkmate who was not previously in checkmate.
 */
int fc_game_opponent_kings_check_status (fc_game_t *game, fc_player_t player,
		fc_move_t *move);

/**
 * @brief Determine if the given move is legal.
 *
 * In this instance, "legal" means a move which meets one of the following
 * conditions:
 * 	-# It is an available move which does not put the current player's king
 * 	into check unless the king is in checkmate.
 * 	-# If the player is in checkmate, then he may move any piece EXCEPT the
 * 	king.
 * 	-# If the player has no available moves, then the move must allow him
 * 	to remove a piece which does not put him in check (unless he's in
 * 	checkmate).
 * 	-# It is the removal of the king if and only if no moves are possible
 * 	and the king is the only piece that player controls on the board.
 *
 * @param[in] game A pointer to the game.
 * @param[in] move The move in question.
 *
 * @return 1 if the move is legal; 0 otherwise
 */
int fc_game_is_move_legal (fc_game_t *game, fc_move_t *move);

/**
 * @brief Determines if the game is over.
 *
 * @param[in] game A pointer to the game.
 *
 * @return 1 if any player and his partner's kings have been captured; 0
 * otherwise
 */
int fc_game_is_over (fc_game_t *game);

/**
 * @brief Update the game with the new move.
 *
 * @remark Simply calls fc_board_make_move(), so please see its documentation
 * for details.
 *
 * @param[in,out] game A pointer to the game.
 * @param[in] move The move to make.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_game_make_move (fc_game_t *game, fc_move_t *move);

/**
 * @brief Sets the piece that the pawn should be promoted to with the given
 * move.
 *
 * @param[out] move The move we want to set the promoted piece for.
 * @param[in] promote The piece to promote the pawn to.
 *
 * @return void
 */
void fc_game_set_promote_pawn (fc_move_t *move, fc_piece_t promote);

/**
 * @brief Translates a forchess move struct into a set of board coordinates.
 *
 * The coordinates come in the form of two x and y coordinates.  The x's
 * represent the columns commonly labelled 'a' through 'h' and the y's, rows
 * '1' through '8'.  The values which are returned are [0, 7] inclusive.
 *
 * @param[in] game A pointer to the game.
 * @param[out] x1 The x coord that the piece is on.
 * @param[out] y1 The y coord that the piece is on.
 * @param[out] x2 The x coord that the piece is moving to.
 * @param[out] y2 The y coord that the piece is moving to.
 * @param[in] move The move we are translating.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_game_convert_move_to_coords (fc_game_t *game, int *x1, int *y1,
		int *x2, int *y2, fc_move_t *move);

/**
 * @brief Translates (x, y) coordinates into a move.
 *
 * The coordinates should be written in the form of two pairs of (x, y)
 * coordinates that represent where a piece is (x1, y1) and where the piece
 * should be moved to (x2, y2).
 *
 * @param[in] game A pointer to the game.
 * @param[out] move The move which will be set.
 * @param[in] x1 The x coord that the piece is on.
 * @param[in] y1 The y coord that the piece is on.
 * @param[in] x2 The x coord that the piece is moving to.
 * @param[in] y2 The y coord that the piece is moving to.
 *
 * @return 1 on success; 0 otherwise
 */
int fc_game_convert_coords_to_move (fc_game_t *game, fc_move_t *move,
		int x1, int y1, int x2, int y2);

#endif
