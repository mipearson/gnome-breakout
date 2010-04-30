/*
 * Collision functions. This has alot of stuff that needs fixing.
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include <time.h>
#include <math.h>
#include "breakout.h"
#include "ball.h"
#include "collision.h"
#include "block.h"
#include "powerup.h"

/* Use these for brevity in check_collision */
#define AX1 (one->geometry.x1)
#define AX2 (one->geometry.x2)
#define BX1 (two->geometry.x1)
#define BX2 (two->geometry.x2)
#define AY1 (one->geometry.y1)
#define AY2 (one->geometry.y2)
#define BY1 (two->geometry.y1)
#define BY2 (two->geometry.y2)

/* Radians macros */
#define RAD180 3.1415926535
#define RAD360 (RAD180 * 2.0)
#define RAD90 (RAD180 / 2.0)
#define RAD30 (RAD90 / 3.0)
#define RAD270 (RAD180 + RAD90)

#define BAT_LOW_CAP (RAD180 - (RAD90 - RAD30)) 
#define BAT_HIGH_CAP (RAD180 + (RAD90 - RAD30)) 

/* Internal functions */
static gboolean check_collision(Entity *one, Entity *two);
static Side find_hit_side(Ball *ball, Entity *one);
static Side find_block_hit_side(Game *game, Block *block, Ball *ball);
static void recalculate_ball_trajectory(Game *game, Ball *ball, Side side);
static void add_bounce_entropy(Game *game, Ball *ball);

/* Checks for a collision between two objects */
static gboolean check_collision(Entity *one, Entity *two) {
	gboolean xcollision;
	gboolean ycollision;

	xcollision = ((AX1 >= BX1 && AX1 <= BX2) 
		|| (AX2 >= BX1 && AX2 <= BX2)
		|| (AX1 <= BX1 && AX2 >= BX2));
	ycollision = ((AY1 >= BY1 && AY1 <= BY2) || (AY2 >= BY1 && AY2 <= BY2) || (AY1 <= BY1 && AY2 >= BY2));

	if(xcollision && ycollision) {
		return TRUE;
	}

	return FALSE;
}

/* Finds which side of an object has been hit. Assumes that a collision has
 * actually occured */
static Side find_hit_side(Ball *ball, Entity *one) {
	gint x1, x2, y1, y2;
	Side side = SIDE_NONE;
	
	/* First, find out where the ball was -before- it hit */
	x1 = ball->geometry.x1 - ((gdouble) ball->speed * sin(ball->direction));
	x2 = ball->geometry.x2 - ((gdouble) ball->speed * sin(ball->direction));
	y1 = ball->geometry.y1 - ((gdouble) ball->speed * cos(ball->direction));
	y2 = ball->geometry.y2 - ((gdouble) ball->speed * cos(ball->direction));

	/* Now, find out which side was hit based on where the ball was */
	if((x1 > AX1 && x1 < AX2) || (x2 > AX1 && x2 < AX2) || (x1 < AX1 && x2 > AX2)) {
		if(y2 < AY1)
			side = SIDE_TOP;
		else if(y1 > AY2)
			side = SIDE_BOTTOM;
		else
			side = SIDE_DIAGONAL;
	} else if((y1 > AY1 && y1 < AY2) || (y2 > AY1 && y2 < AY2) || (y1 < AY1 && y2 > AY2)) {
		if(x2 < AX1)
			side = SIDE_LEFT;
		else if(x1 > AX2)
			side = SIDE_RIGHT;
		else
			side = SIDE_DIAGONAL;
	} else { 
		side = SIDE_DIAGONAL;
	}

	return side;
}

/* Changes a balls trajectory depending on which side of an object it hit. */
static void recalculate_ball_trajectory(Game *game, Ball *ball, Side side) {

	add_bounce_entropy(game, ball);

	/* Recalculate the trajectory */
	switch(side) {
		case SIDE_RIGHT :
			ball->direction += (RAD90 - ball->direction) * 2.0;
			ball->direction += RAD180;
			break;
		case SIDE_LEFT :
			ball->direction += (RAD270 - ball->direction) * 2.0; 
			ball->direction += RAD180;
			break;
		case SIDE_BOTTOM :
			ball->direction += (RAD180 - ball->direction) * 2.0; 
			ball->direction += RAD180;
			break;
		case SIDE_TOP :
			/*if(ball->direction > RAD180)
				ball->direction -= ball->direction * 2.0;
			else*/
				ball->direction += (RAD360 - ball->direction) * 2.0;
			ball->direction += RAD180;
			break;
		case SIDE_DIAGONAL :
			ball->direction += RAD180;
			break;
		default :
			g_assert_not_reached();
	}

	/* Make sure the ball hasn't been bounced the wrong way */
	while(ball->direction > RAD360)
		ball->direction -= RAD360;

	while(ball->direction < 0)
		ball->direction += RAD360;

	/* And move it an extra step, so that the ball doesn't appear inside
	 * the block when we refresh */
	move_ball(ball);
}

/* Checks whether the ball hit the block, and then adjusts accordingly. Returns
 * TRUE if a collision occured */
gboolean ball_block_collision(Game *game, Ball *ball) {
	Side side;
	Block *block;
	gboolean rval = FALSE;

	block = find_block_from_position(game, (Entity *) ball);	
	if(block) {
		hit_block(game, block);
		side = find_hit_side(ball, (Entity *) block);
		if(side == SIDE_DIAGONAL)
			side = find_block_hit_side(game, block, ball);
		increase_ball_speed(game, ball);
		recalculate_ball_trajectory(game, ball, side);
		rval = TRUE;
	}
	return rval;
}

/* Checks whether the ball hit the wall, and then adjusts accordingly. Returns
 * TRUE if the ball has died, not if a collision occurs. */
gboolean ball_wall_collision(Game *game, Ball *ball) {
	gboolean ball_dead = FALSE;

	if(ball->geometry.x1 < 0) {
		if(ball->geometry.y1 < 0) {
			recalculate_ball_trajectory(game, ball, SIDE_DIAGONAL);
		} else {
			recalculate_ball_trajectory(game, ball, SIDE_RIGHT);
		}
	} else if(ball->geometry.x2 > GAME_WIDTH) {
		if(ball->geometry.y1 < 0) {
			recalculate_ball_trajectory(game, ball, SIDE_DIAGONAL);
		} else {
			recalculate_ball_trajectory(game, ball, SIDE_LEFT);
		}
	} else if(ball->geometry.y1 < 0) {
		recalculate_ball_trajectory(game, ball, SIDE_BOTTOM);
	} else if(ball->geometry.y2 > GAME_HEIGHT) {
		ball_dead = TRUE;
	}

	/* Make sure the ball isn't -still- outside the boundaries */
	/*if(check_ball) {
		if(ball->geometry.x1 < 0) {
			ball->geometry.x1 = 0;
			ball->geometry.x2 = BALL_WIDTH;
		}
		if(ball->geometry.x2 > GAME_WIDTH) {
			ball->geometry.x1 = GAME_WIDTH - BALL_WIDTH;
			ball->geometry.x2 = GAME_WIDTH;
		}
		if(ball->geometry.y1 < 0) {
			ball->geometry.y1 = 0;
			ball->geometry.y2 = BALL_HEIGHT;
		}
	}*/

	return ball_dead;
}

/* Checks whether the bat 'caught' a powerup, and acts accordingly. Returns
 * TRUE if a collision occured */
gboolean bat_powerup_collision(Game *game, Powerup *powerup) {
	if(check_collision((Entity *) game->bat, (Entity *) powerup)) {
		activate_powerup(game, powerup);
		return TRUE;
	}

	return FALSE;
}

/* Checks whether the ball hit the bat, and then adjusts the ball's trajectory
 * accordingly. Returns TRUE if a collision occured */
gboolean ball_bat_collision(Ball *ball, Bat *bat) {
	Side side;
	gint ballpos, batpos;
	gdouble ballper, ballrad;

	if(check_collision((Entity *) ball, (Entity *) bat)) {
		side = find_hit_side(ball, (Entity *) bat);
		if(side != SIDE_BOTTOM) {
			ballpos = ball->geometry.x1 + BALL_WIDTH / 2;
			batpos = bat->geometry.x1 + bat->width / 2;
			ballper = (double) (batpos - ballpos) /
					(bat->width / 2);
			ballrad = ballper * RAD30 + RAD180;
			ball->direction += (ballrad - ball->direction) * 2.0; 
			ball->direction += RAD180;
			while(ball->direction > RAD360)
				ball->direction -= RAD360;

			/* Make sure the ball hasn't been redirected at too 
			 * low an angle */
			if(ball->direction < BAT_LOW_CAP)
				ball->direction = BAT_LOW_CAP;
			else if(ball->direction > BAT_HIGH_CAP)
				ball->direction = BAT_HIGH_CAP;

			move_ball(ball);
		} else {
			/* The ball should never rebound off of the bottom wall
			 * , therefore this should never happen */
			g_assert_not_reached();
		}

		return TRUE;
	}

	return FALSE;
}

/* Attempts to determine what side was hit if a standard find_hit_side
 * returns SIDE_DIAGONAL. The result is based on the neighbours of the block
 */
static Side find_block_hit_side(Game *game, Block *block, Ball *ball) {
	gint x1, y1, block_x, block_y, ball_x, ball_y;
	gboolean ball_is_above, ball_is_left;
	gboolean above, below, left, right;
	Side ret = SIDE_DIAGONAL;

	/* First, find out where the ball was -before- it hit */
	x1 = ball->geometry.x1 - ((gdouble) ball->speed * sin(ball->direction));
	y1 = ball->geometry.y1 - ((gdouble) ball->speed * cos(ball->direction));
	
	/* Now get an approximation of where the ball is coming from. Instead
	 * of using standard collision detection methods, we'll base it on
	 * the center of each object */
	block_x = block->geometry.x1 + BLOCK_WIDTH / 2;
	block_y = block->geometry.y1 + BLOCK_HEIGHT / 2;
	ball_x = x1 + BALL_WIDTH / 2;
	ball_y = y1 + BALL_HEIGHT / 2;

	if(ball_x < block_x)
		ball_is_left = TRUE;
	else
		ball_is_left = FALSE;

	if(ball_y < block_y)
		ball_is_above = TRUE;
	else
		ball_is_above = FALSE;

	/* Find out the neighbours of the block */	
	above = block_has_neighbour(game, block, SIDE_TOP);
	below = block_has_neighbour(game, block, SIDE_BOTTOM);
	left = block_has_neighbour(game, block, SIDE_LEFT);
	right = block_has_neighbour(game, block, SIDE_RIGHT);

	if(ball_is_above && ball_is_left) {
		if(above && !left)
			ret = SIDE_LEFT;
		else if(left && !above)
			ret = SIDE_TOP;
		else
			ret = SIDE_DIAGONAL;
	} else if(ball_is_above && !ball_is_left) {
		if(above && !right)
			ret = SIDE_RIGHT;
		else if(right && !above)
			ret = SIDE_TOP;
		else
			ret = SIDE_DIAGONAL;
	} else if(!ball_is_above && ball_is_left) {
		if(below && !left)
			ret = SIDE_LEFT;
		else if(left && !below)
			ret = SIDE_BOTTOM;
		else
			ret = SIDE_DIAGONAL;
	} else if(!ball_is_above && !ball_is_left) {
		if(below && !right)
			ret = SIDE_RIGHT;
		else if(right && !below)
			ret = SIDE_BOTTOM;
		else
			ret = SIDE_DIAGONAL;
	} else {
		g_assert_not_reached();
	}

	return ret;
}

/* Applies the bounce entropy to a ball's trajectory. */
static void add_bounce_entropy(Game *game, Ball *ball) {
	gdouble diff;

	diff = (gdouble) game->flags->bounce_entropy / 100;
	diff *= RAD180;
	diff *= ((gdouble) rand() / RAND_MAX - 0.5);

	ball->direction += diff;
}
