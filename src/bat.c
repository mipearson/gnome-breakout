/*
 * Bat handling functions
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include "breakout.h"
#include "bat.h"
#include "block.h"
#include "anim.h"
#include "gui.h"

#define LASER_SPEED 19
#define LASER_WIDTH 15
#define LASER_HEIGHT 15

/* Internal functions */
static void move_bat(Game *game);
static void change_to_laser(Bat *bat);
static void change_to_wide(Bat *bat);
static void reset_from_laser(Bat *bat);
static void reset_from_wide(Bat *bat);
static void iterate_bat_laser(Game *game);
static void remove_child_laser(Bat *bat, Entity *child);
static void destroy_children_laser(Bat *bat);

static void move_bat(Game *game) {
        Bat *bat;
        gint bat_move;

        /* Included for brevity */
        bat = game->bat;

        if(game->flags->keyboard_control) {
                bat_move = game->keyboard_move;
                if(bat->geometry.x1 + bat_move > 0
                                && bat->geometry.x2 + bat_move < GAME_WIDTH) {
                        bat->geometry.x1 += bat_move;
                        bat->geometry.x2 += bat_move;
                } else {
                        if(bat->geometry.x2 + bat_move >= GAME_WIDTH) {
                                bat->geometry.x2 = GAME_WIDTH;
                                bat->geometry.x1 = GAME_WIDTH - bat->width;
                        } else {
                                bat->geometry.x1 = 0;
                                bat->geometry.x2 = bat->width;
                        }
                }
        } else if(game->flags->mouse_control) {
                bat_move = game->mouse_move;
                if(bat_move < bat->width / 2)
                        bat_move = bat->width /2;
                else if(bat_move > GAME_WIDTH - bat->width /2)
                        bat_move = GAME_WIDTH - bat->width /2;

                bat->geometry.x1 = bat_move - bat->width / 2;
                bat->geometry.x2 = bat->geometry.x1 + bat->width;
        }
        update_canvas_position((Entity *) bat);
}

/* Creates a new bat at the start of a game */
void new_bat(Game *game) {
	Bat *bat;

        bat = g_malloc(sizeof(Bat));
        g_assert(bat);

	bat->width = BAT_WIDTH;
        bat->geometry.y2 = GAME_HEIGHT - BLOCK_WALL_PADDING;
        bat->geometry.y1 = bat->geometry.y2 - BAT_HEIGHT;
        bat->geometry.x1 = GAME_WIDTH / 2 - bat->width / 2;
        bat->geometry.x2 = bat->geometry.x1 + bat->width;
        g_assert(bat->geometry.x1 > BLOCK_WALL_PADDING);
        g_assert(bat->geometry.x2 < GAME_WIDTH - BLOCK_WALL_PADDING);

        bat->animation = get_static_animation(ANIM_BAT_DEFAULT);
	bat->children = NULL;
        add_to_canvas((Entity *) bat);
        bat->type = BAT_DEFAULT;
	bat->num_lasers = 0;

	game->bat = bat;
}

/* Iterates the bat */
void iterate_bat(Game *game) {
	switch(game->bat->type) {
		case BAT_WIDE :
		case BAT_DEFAULT :
			move_bat(game);
			break;
		case BAT_LASER :
			move_bat(game);
			iterate_bat_laser(game);
			break;
		default :
			g_assert_not_reached();
	}
}

/* Changes the bat type. Don't use this to change a bat back to DEFAULT, use
 * reset_bat_type instead */
void change_bat_type(Game *game, BatType type) {
	/* Special case */
	if(game->bat->type == BAT_LASER && type == BAT_LASER) {
		game->bat->num_lasers_allowed++;
		return;
	}

	if(game->bat->type != BAT_DEFAULT)
		reset_bat_type(game);

	switch(type) {
		case BAT_DEFAULT :
			g_warning("DEVELOPER: Don't use this! Use reset_bat_type instead!");
			reset_bat_type(game);
			break;
		case BAT_LASER :
			change_to_laser(game->bat);
			break;
		case BAT_WIDE :
			change_to_wide(game->bat);
			break;
		default :
			g_assert_not_reached();
	}
}

/* Resets the bat back to BAT_DEFAULT. Handles the destruction of anything
 * that a non-standard bat type may spawn */
void reset_bat_type(Game *game) {
	switch(game->bat->type) {
		case BAT_DEFAULT :
			break;
		case BAT_LASER :
			reset_from_laser(game->bat);
			break;
		case BAT_WIDE :
			reset_from_wide(game->bat);
			break;
		default :
			g_assert_not_reached();
	}
}

/* Destroys the bat, de-allocating its resources and suchlike */
void destroy_bat(Game *game) {
	reset_bat_type(game);
	remove_from_canvas((Entity *) game->bat);
	g_free(game->bat);
	game->bat = NULL;
}

static void reset_from_laser(Bat *bat) {
	destroy_children_laser(bat);
	bat->num_lasers_allowed = 0;

	remove_from_canvas((Entity *) bat);
	bat->animation = get_animation(ANIM_BAT_DEFAULT);
	add_to_canvas((Entity *) bat);

	bat->type = BAT_DEFAULT;
}

static void reset_from_wide(Bat *bat) {
	bat->width = BAT_WIDTH;
	bat->geometry.x1 += (BAT_WIDE_WIDTH - BAT_WIDTH) / 2;
	bat->geometry.x2 = bat->geometry.x1 + bat->width;
	bat->type = BAT_DEFAULT;

	remove_from_canvas((Entity *) bat);
	bat->animation = get_animation(ANIM_BAT_DEFAULT);
	add_to_canvas((Entity *) bat);
}

static void change_to_wide(Bat *bat) {
	bat->width = BAT_WIDE_WIDTH;
	bat->geometry.x1 -= (BAT_WIDE_WIDTH - BAT_WIDTH) / 2;
	if(bat->geometry.x1 < 0)
		bat->geometry.x1 = 0;
	bat->geometry.x2 = bat->geometry.x1 + bat->width;
	bat->type = BAT_WIDE;

	remove_from_canvas((Entity *) bat);
	bat->animation = get_animation(ANIM_BAT_WIDE);
	add_to_canvas((Entity *) bat);
}

static void change_to_laser(Bat *bat) {
	remove_from_canvas((Entity *) bat);
	bat->animation = get_animation(ANIM_BAT_LASER);
	add_to_canvas((Entity *) bat);
	bat->num_lasers = 0;
	bat->num_lasers_allowed = 1;

	bat->type = BAT_LASER;
}

static void iterate_bat_laser(Game *game) {
	gboolean kill_laser = FALSE;
	gint bat_center;
	Block *block;
	GList *children;
	Entity *laser;

	children = game->bat->children;
	while(children) { 
		laser = (Entity *) children->data;
		children = g_list_next(children);
		laser->geometry.y1 -= LASER_SPEED;
		laser->geometry.y2 -= LASER_SPEED;

		if(laser->geometry.y1 < 0) {
			kill_laser = TRUE;
		} else if((block = find_block_from_position(game, laser))) {
			if(block->type != BLOCK_DEAD) {
				hit_block(game, block);
				kill_laser = TRUE;
			}
		}

		if(kill_laser) {
			remove_child_laser(game->bat, laser);
		} else {
			update_canvas_position(laser);
		}
	}

	if(game->fire1_pressed && game->bat->num_lasers_allowed > game->bat->num_lasers ) {
		bat_center = game->bat->geometry.x1
			+ game->bat->width / 2;

		laser = g_malloc(sizeof(Entity));
		laser->geometry.x1 = bat_center - LASER_WIDTH / 2;
		laser->geometry.x2 = laser->geometry.x1 + LASER_WIDTH;
		laser->geometry.y2 = game->bat->geometry.y1;
		laser->geometry.y1 = laser->geometry.y2 - LASER_HEIGHT;

		laser->animation = get_animation(ANIM_LASER);
		add_to_canvas(laser);
		game->bat->num_lasers++;
		game->bat->children = g_list_prepend(game->bat->children, laser);
	}
}

/* Removes a laser entity from the game */
void remove_child_laser(Bat *bat, Entity *child) {
	remove_from_canvas(child);
	bat->num_lasers--;
	bat->children = g_list_remove(bat->children, child);
	g_free(child);
}

/* Destroys a children list of lasers. */
void destroy_children_laser(Bat *bat) {
	GList *children;
	Entity *laser;

	children = bat->children;
	while(children) {
		laser = (Entity *) children->data;
		children = g_list_next(children);
		g_list_remove(bat->children, laser);
		remove_from_canvas(laser);
		g_free(laser);
	}

	bat->children = NULL;
	bat->num_lasers = 0;
}

