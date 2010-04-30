/*
 * Powerup handling
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include <stdlib.h>
#include "breakout.h"
#include "gui.h"
#include "anim.h"
#include "game.h"
#include "ball.h"
#include "bat.h"
#include "collision.h"
#include "powerup.h"

#define POWERUP_SPEED 2

/* Note that it's better to have a low chance of a powerup appearing and
 * powerful powerups, rather than a high chance and weak powerups. This is
 * mainly because powerups cause slowdown :) */
#define POWERUP_CHANCE 20.0

/* The chance that a powerup will occur. Works like a D20 roll in AD&D. If
 * the roll is higher than the probability, a powerup of that probability will
 * occur */
#define PROBABILITY_CAP 20.0
#define LOW_PROBABILITY 16
#define MEDIUM_PROBABILITY 9
#define HIGH_PROBABILITY 0
static PowerupType low_probability[] =
		{ POWER_NEXTLEVEL, POWER_NEWLIFE, -1 };
static PowerupType medium_probability[] =
		{ POWER_SLOW, POWER_WIDEBAT, POWER_NEWBALL, -1 };
static PowerupType high_probability[] =
		{ POWER_SCORE500, POWER_LASER, -1 };

/* Internal functions */
static GList *remove_powerup(GList *powerups, Powerup *powerup);
static void move_powerup(Powerup *powerup);

/* Checks to see whether to create a powerup, and if so, creates one */
void new_powerup(Game *game, int x, int y) {
	Powerup *powerup;
	gint rand_result;
	gint num_powerups;
	PowerupType *p_section = NULL;

	if((POWERUP_CHANCE * rand()/RAND_MAX) > 1.0)
		return;

	powerup = g_malloc(sizeof(Powerup));
	g_assert(powerup);

	powerup->geometry.x1 = x;
	powerup->geometry.y1 = y;
	powerup->geometry.x2 = x + POWERUP_WIDTH;
	powerup->geometry.y2 = y + POWERUP_HEIGHT;

	rand_result = (int) (PROBABILITY_CAP * rand() / RAND_MAX);
	if(rand_result >= LOW_PROBABILITY)
		p_section = low_probability;
	else if(rand_result >= MEDIUM_PROBABILITY)
		p_section = medium_probability;
	else if(rand_result >= HIGH_PROBABILITY)
		p_section = high_probability;
	else
		g_assert_not_reached();

	for(num_powerups = 0; p_section[num_powerups] != -1; num_powerups++);
	rand_result = (int) ((double) num_powerups * rand() / RAND_MAX);
	powerup->type = p_section[rand_result];

	switch(powerup->type) {
		case POWER_SCORE500 :
			powerup->animation = get_static_animation
					(ANIM_POWERUP_SCORE500);
			break;
		case POWER_LASER :
			powerup->animation = get_static_animation
					(ANIM_POWERUP_LASER);
			break;
		case POWER_NEWLIFE :
			powerup->animation = get_static_animation
					(ANIM_POWERUP_NEWLIFE);
			break;
		case POWER_NEWBALL :
			powerup->animation = get_static_animation
					(ANIM_POWERUP_NEWBALL);
			break;
		case POWER_NEXTLEVEL :
			powerup->animation = get_static_animation
					(ANIM_POWERUP_NEXTLEVEL);
			break;
		case POWER_SLOW :
			powerup->animation = get_static_animation
					(ANIM_POWERUP_SLOW);
			break;
		case POWER_WIDEBAT :
			powerup->animation = get_static_animation
					(ANIM_POWERUP_WIDEBAT);
			break;
		default :
			g_assert_not_reached();
	}

	add_to_canvas((Entity *) powerup);
	game->powerups = g_list_prepend(game->powerups, powerup);
}

/* Adtivates a powerup, and removes it */
void activate_powerup(Game *game, Powerup *powerup) {
	switch(powerup->type) {
		case POWER_SCORE500 :
			ADD_SCORE(game, 500);
			break;
		case POWER_LASER :
			change_bat_type(game, BAT_LASER);
			break;
		case POWER_NEWLIFE :
			new_life(game);
			break;
		case POWER_NEWBALL :
			new_ball_stuck(game);
			break;
		case POWER_NEXTLEVEL :
			game->powerup_next_level = TRUE;
			break;
		case POWER_SLOW :
			slow_balls(game);
			break;
		case POWER_WIDEBAT :
			change_bat_type(game, BAT_WIDE);
			break;
		default :
			g_assert_not_reached();
	}

	if(powerup)
		game->powerups = remove_powerup(game->powerups, powerup);
}

static GList *remove_powerup(GList *powerups, Powerup *powerup) {
        powerups = g_list_remove(powerups, powerup);
        remove_from_canvas((Entity *) powerup);
        g_free(powerup);

        return powerups;
}

/* De-allocate a list of powerups */
void destroy_powerup_list(GList *powerups) {
	while(powerups)
		powerups = remove_powerup(powerups, (Powerup *) powerups->data);
}

/* Make the powerups move, and see if the bat caught them */
void iterate_powerups(Game *game) {
	GList *powerups;
	Powerup *powerup;

	powerups = game->powerups;
	while(powerups) {
		powerup = (Powerup *) powerups->data;
		powerups = g_list_next(powerups);
		move_powerup(powerup);

		if(!bat_powerup_collision(game, powerup))
			if(powerup->geometry.y2 > GAME_HEIGHT)
				game->powerups = remove_powerup(game->powerups, powerup);
	}
}

static void move_powerup(Powerup *powerup) {
	powerup->geometry.y1 += POWERUP_SPEED;
	powerup->geometry.y2 += POWERUP_SPEED;
	update_canvas_position((Entity *)powerup);
}
