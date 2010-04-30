/*
 * Functions that generate new levels
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

Level *generate_level(gint level_num);
void hit_block(Game *game, Block *block);
void destroy_level(Game *game);
void destroy_block(Game *game, Block *block);
gboolean check_level_end(Game *game);
void iterate_blocks(Game *game);
Block *find_block_from_position(Game *game, Entity *entity);
gboolean block_has_neighbour(Game *game, Block *block, Side side);
