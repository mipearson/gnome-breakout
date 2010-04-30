/*
 * Ball handling functions
 * I'll try not to make too many crap jokes. Heh heh. Ball handling.
 *
 * Oh, and thanks to moonlite of oz.org #coders for identifying a bug in
 * iterate_balls()
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include <stdlib.h>
#include <math.h>
#include "breakout.h"
#include "gui.h"
#include "game.h"
#include "anim.h"
#include "ball.h"
#include "collision.h"
#include "block.h"

#define PI 3.14159265
#define DEFAULT_DIRECTION PI
#define FIRE1_DIRECTION (PI + PI / 4.0)
#define FIRE2_DIRECTION (PI - PI / 4.0)
#define MAX_AIRTIME 1000 /* Twenty seconds at 50 FPS */

/* Internal Functions */
static void iterate_ball_default(Game *game, Ball *ball);
static void iterate_ball_stuck(Game *game, Ball *ball);
static void ball_default_die(Game *game, Ball *ball);
static GList *remove_ball(GList *balls, Ball *ball);

/* Creates a new ball. ATM, we only have one per game, but the Game structure
 * is designed in such a way to allow multiple balls */
void new_ball_stuck(Game *game) {
	Ball *ball;
	gint x, y;

	ball = g_malloc(sizeof(Ball));
	g_assert(game->bat);
	x = game->bat->geometry.x1 + BAT_WIDTH / 2;
	y = game->bat->geometry.y1 - BALL_HEIGHT / 2;
	y--;

	ball->geometry.x1 = x - BALL_WIDTH / 2;
	ball->geometry.x2 = ball->geometry.x1 + BALL_WIDTH;
	ball->geometry.y1 = y - BALL_HEIGHT / 2;
	ball->geometry.y2 = ball->geometry.y1 + BALL_HEIGHT;
	ball->pseudo_x1 = ball->geometry.x1;
	ball->pseudo_y1 = ball->geometry.y1;
	ball->airtime = 0;

	ball->animation = get_static_animation(ANIM_BALL_DEFAULT);
	ball->speed = 0;
	ball->direction = 0;

	ball->type = BALL_STUCK;
	add_to_canvas((Entity *) ball);
	game->balls = g_list_prepend(game->balls, ball);
}

/* Destroys a ball. Assumes that the ball has either fallen off of the screen,
 * or hit some sort of "bad" block or somesuch. Will handle loss of life and
 * endgame */
void ball_die(Game *game, Ball *ball) {
	switch(ball->type) {
		case BALL_STUCK :
		case BALL_DEFAULT :
			ball_default_die(game, ball);
			break;
		default :
			g_assert_not_reached();
	}
}

/* Unlike powerup.c, the functions here must actually remove the entity */
static void ball_default_die(Game *game, Ball *ball) {
	game->balls = remove_ball(game->balls, ball);
}

static GList *remove_ball(GList *balls, Ball *ball) {
	balls = g_list_remove(balls, ball);
	remove_from_canvas((Entity *) ball);
	g_free(ball);

	return balls;
}

/* If the game ends by loss of life, this shouldn't need to be run. */
void destroy_ball_list(GList *balls) {
	while(balls)
		balls = remove_ball(balls, (Ball *) balls->data);
}

/* Make the balls move, and handle collisions and such */
void iterate_balls(Game *game) {
	GList *balls;
	Ball *ball;

	balls = game->balls;
	while(balls) {
		ball = (Ball *) balls->data;
		balls = g_list_next(balls);

		switch(ball->type) {
			case BALL_DEFAULT :
				iterate_ball_default(game, ball);
				break;
			case BALL_STUCK :
				iterate_ball_stuck(game, ball);
				break;
			default :
				g_assert_not_reached();
		}

	}
}

static void iterate_ball_default(Game *game, Ball *ball) {
	Block *block;
	gint old_x1, old_y1;
	gdouble old_direction;
	gboolean lost_life = FALSE;

	old_x1 = ball->geometry.x1;
	old_y1 = ball->geometry.y1;
	old_direction = ball->direction;

	move_ball(ball);

	/* Check for collision with blocks */
	if(!ball_block_collision(game, ball)) {
	/* If no blocks hit, check for collision with everything else */
		if(ball_bat_collision(ball, game->bat)) {
			ball->airtime = 0;
		} 
		lost_life = ball_wall_collision(game, ball);
	}

	if(lost_life) {
		ball_die(game, ball);
	} else {
		/* Makes sure the ball hasn't stayed in the air to long. If it
		 * has, randomise the direction. This should help prevent the
		 * ball getting 'stuck' between invincible blocks */
		if(ball->airtime < MAX_AIRTIME) {
			ball->airtime++;
		} else {
 			ball->direction = (PI * 2.0 * rand() / RAND_MAX);  
			ball->airtime = 0;
		}
	
		/* This is a bit of a hack, but it's the best way I can think
		 * of to stop the stuck-in-block bug. */
		if(old_x1 == ball->geometry.x1 
				&& old_y1 == ball->geometry.y1
				&& old_direction == ball->direction) {
			block = find_block_from_position(game, (Entity *) ball);
			if(block) {
				destroy_block(game, block);
			} else {
				g_assert_not_reached();
			}
		}
	}
}

/* The bat must be moved before the ball, or the stuck ball will appear
 * flickery. Also, this function will reset fire1_pressed or fire2_pressed,
 * to stop multiple balls being launched at once */
static void iterate_ball_stuck(Game *game, Ball *ball) {
	gint bat_center;

	bat_center = game->bat->geometry.x1 + BAT_WIDTH / 2;
	ball->geometry.x1 = bat_center - BALL_WIDTH / 2;
	ball->geometry.x2 = ball->geometry.x1 + BALL_WIDTH;
	ball->pseudo_x1 = ball->geometry.x1;
	ball->pseudo_y1 = ball->geometry.y1;

	update_canvas_position((Entity *) ball);

	if(game->fire1_pressed) {
		ball->type = BALL_DEFAULT;
		ball->speed = game->flags->ball_initial_speed;
		ball->direction = FIRE1_DIRECTION;
		ball->airtime = 0;
		game->fire1_pressed = FALSE;
	} else if (game->fire2_pressed) {
		ball->type = BALL_DEFAULT;
		ball->speed = game->flags->ball_initial_speed;
		ball->direction = FIRE2_DIRECTION;
		ball->airtime = 0;
		game->fire2_pressed = FALSE;
	}
}

/* Moves the ball one frame */
void move_ball(Ball *ball) {
	ball->pseudo_x1 += ball->speed * sin(ball->direction);
	ball->pseudo_y1 += ball->speed * cos(ball->direction);


	ball->geometry.x1 = (gint) ball->pseudo_x1;
	ball->geometry.y1 = (gint) ball->pseudo_y1;
	ball->geometry.x2 = ball->geometry.x1 + BALL_WIDTH;
	ball->geometry.y2 = ball->geometry.y1 + BALL_HEIGHT;
	update_canvas_position((Entity *) ball);
}

/* Increases the speed of the ball */
void increase_ball_speed(Game *game, Ball *ball) {
	if(ball->speed < game->flags->ball_max_speed)
		ball->speed += game->flags->ball_speed_increment;
}

/* Slows all of the balls down to half their initial speed */
void slow_balls(Game *game) {
	GList *balls;
	Ball *ball;

	for(balls = game->balls; balls; balls = g_list_next(balls)) {
		ball = (Ball *) balls->data;

		if(ball->type != BALL_STUCK) {
			ball->speed = game->flags->ball_initial_speed / 2;
		}
	}
}
