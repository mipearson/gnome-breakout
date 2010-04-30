/*
 * Collision detection
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

gboolean ball_block_collision(Game *game, Ball *ball);
gboolean ball_wall_collision(Game *game, Ball *ball);
gboolean bat_powerup_collision(Game *game, Powerup *powerup);
gboolean ball_bat_collision(Ball *ball, Bat *bat);
