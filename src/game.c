/*
 * Main game iteration, and misc functions
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include <sys/time.h>
#include <unistd.h>

#include "breakout.h"
#include "gui.h"
#include "game.h"
#include "anim.h"
#include "block.h"
#include "ball.h"
#include "bat.h"
#include "powerup.h"
#include "flags.h"
#include "leveldata.h"

#define NUM_LIVES 5

/* Internal Variables */
static gboolean left_ispressed = FALSE;
static gboolean right_ispressed = FALSE;

#define FRAMES_PER_SECOND 50
#define USEC_PER_SEC 1000000
#define USEC_PER_FRAME (USEC_PER_SEC / FRAMES_PER_SECOND)

#define NEWLIFESCORE 20000
#define NEXTLEVELSCORE 5000

void iterate_game(Game * game)
{
	struct timeval start_tv, end_tv;
	struct timezone tz;
	gint32 diff_t;
	int stop;

	while (game->state == STATE_RUNNING) {
		gettimeofday(&start_tv, &tz);
		game->mouse_move = get_mouse_x_position();

		iterate_bat(game);
		iterate_balls(game);
		iterate_powerups(game);
		iterate_blocks(game);
	
		stop = 0;

		process_events(game);

        	gui_update_game(game);

		game->fire1_pressed = FALSE;
		game->fire2_pressed = FALSE;

		process_gnome_events();

		gettimeofday(&end_tv, &tz);
		diff_t = ((start_tv.tv_sec - end_tv.tv_sec) * USEC_PER_SEC)
		    + (start_tv.tv_usec - end_tv.tv_usec)
		    + USEC_PER_FRAME;

		if (diff_t > 0)
			usleep(diff_t);
	}
}

/* Makes a new game, and starts it up */
void run_game(Game * game)
{
	g_assert(game->state == STATE_STOPPED);

	if (!leveldata_num_levels()) {
		gui_warning("No levels configured!");
		return;
	}
	game->level = generate_level(0);

	game->flags->difficulty = game->flags->next_game_difficulty;
	compute_flags(game->flags);
	game->state = STATE_RUNNING;
	game->balls = NULL;
	new_bat(game);
	new_ball_stuck(game);
	game->powerups = NULL;
	game->score = 0;
	game->lives = NUM_LIVES;
	game->level_no = 0;
	game->fire1_pressed = FALSE;
	game->fire2_pressed = FALSE;
	game->mouse_move = 0;
	game->keyboard_move = 0;
	gui_begin_game();
	iterate_game(game);
}

/* Mechanism for tracking where a pause came from and whether we should
 * actually pause or unpause. */
void pause_game(Game * game, PauseType type, gboolean unpause)
{
	if (game->state != STATE_STOPPED) {
		if (unpause && game->state == STATE_PAUSED) {
			g_assert(game->pause_state);
			game->pause_state &= (~type);
			if (!game->pause_state) {
				game->state = STATE_RUNNING;
				iterate_game(game);
			}
		} else if (!unpause) {
			game->pause_state |= type;
			if (game->state == STATE_RUNNING) {
				game->state = STATE_PAUSED;
			}
		}
	} else {
		g_assert(!game->pause_state);
	}
}

/* Ends the game and de-allocates memory. */
void end_game(Game * game, EndGameStatus status)
{
	gui_end_game(status);

	g_assert(game->state != STATE_STOPPED);

	if (game->state != STATE_STOPPED) {
		game->state = STATE_STOPPED;
		destroy_ball_list(game->balls);
		game->balls = NULL;
		destroy_powerup_list(game->powerups);
		game->powerups = NULL;
		destroy_bat(game);
		if (game->level)
			destroy_level(game);
		game->score = 0;
		game->lives = 0;
		game->level_no = 0;
		game->pause_state = 0;
	}
}

/* Destroy the bat/balls */
void lose_life(Game * game)
{
	reset_bat_type(game);
	game->lives--;
	destroy_powerup_list(game->powerups);
	game->powerups = NULL;
}

/* Set up the player for his next life */

/* Destroys the current level data and loads the next one, then updates the
 * GUI.
 */
void next_level(Game * game)
{
	ADD_SCORE(game, NEXTLEVELSCORE);
	destroy_ball_list(game->balls);
	game->balls = NULL;
	destroy_powerup_list(game->powerups);
	game->powerups = NULL;
	destroy_level(game);
	game->level_no++;
	new_ball_stuck(game);
	game->level = generate_level(game->level_no);
	reset_bat_type(game);
}

/* Key handlers. Called from gui.c */
void key_left_pressed(Game * game)
{
	game->keyboard_move = -(game->flags->bat_speed);
	left_ispressed = TRUE;
}

void key_left_released(Game * game)
{
	if (!right_ispressed) {
		game->keyboard_move = 0;
	}
	left_ispressed = FALSE;
}

void key_right_pressed(Game * game)
{
	game->keyboard_move = game->flags->bat_speed;
	right_ispressed = TRUE;
}

void key_right_released(Game * game)
{
	if (!left_ispressed) {
		game->keyboard_move = 0;
	}
	right_ispressed = FALSE;
}

void key_fire1_pressed(Game * game)
{
	if (game->state == STATE_RUNNING)
		game->fire1_pressed = TRUE;
}

void key_fire1_released(Game * game)
{
	return;
}

void key_fire2_pressed(Game * game)
{
	if (game->state == STATE_RUNNING)
		game->fire2_pressed = TRUE;
	return;
}

void key_fire2_released(Game * game)
{
	return;
}

void new_life(Game * game)
{
	game->lives++;
}

/* Processes pending game conditions 
 *
 * Returns non-zero if the game ended
 */
int process_events(Game *game) {
	// Level End
	if(check_level_end(game) || game->powerup_next_level) {
		game->powerup_next_level = FALSE;
		if(game->level_no < leveldata_num_levels() - 1) {
			next_level(game);
		} else {
			end_game(game, ENDGAME_WIN);
			return 1;
		}
	}

	// Free Life
	if(game->score > game->last_newlife_score + NEWLIFESCORE
		* game->flags->score_modifier) {
		new_life(game);
		game->last_newlife_score = game->score;
	}
	
	// Lose Life
	if(!game->balls) {
		lose_life(game);
		if(game->lives < 0) {
		    	end_game(game, ENDGAME_LOSE);
		    	return 1;
		} else {
			destroy_ball_list(game->balls);
			game->balls = NULL;
			new_ball_stuck(game);
		}
	}

	return 0;
}
