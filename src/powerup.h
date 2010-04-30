/*
 * Powerup handling
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

void new_powerup(Game *game, int x, int y);
void activate_powerup(Game *game, Powerup *powerup);
void destroy_powerup_list(GList *powerups);
void iterate_powerups(Game *game);
