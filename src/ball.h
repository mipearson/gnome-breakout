/*
 * Ball handling functions
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

void new_ball_stuck(Game *game);
void ball_die(Game *game, Ball *ball);
void destroy_ball_list(GList *balls);
void move_ball(Ball *ball);
void iterate_balls(Game *game);
void increase_ball_speed(Game *game, Ball *ball);
void slow_balls(Game *game);
