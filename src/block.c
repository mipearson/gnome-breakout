/*
 * Functions that handle blocks 
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include "breakout.h"
#include "anim.h"
#include "powerup.h"
#include "leveldata.h"
#include "gui.h"
#include "block.h"

/* Internal functions */
static void remove_block(Block **blocks, Block *block);
static Block *new_block(char type, gint block_no);
static void block_default_hit(Game *game, Block *block);
static void block_strong_hit(Game *game, Block *block);
static void iterate_block(Game *game, Block *block);
static void block_explode_hit(Game *game, Block *block);
static Block **get_nearby_blocks(Game *game, Block *block);

/* 
 * This function takes a levels.h level definition and transforms it into a
 * block list.
 */
Level *generate_level(gint level_num) {
	Level *level;
	RawLevel *rawlevel;
	gint i; 

	level = g_malloc(sizeof(Level));
	level->blocks_left = 0;

	rawlevel = leveldata_get(level_num);
	for(i = 0; i < BLOCKS_TOTAL; i++) {
		switch(rawlevel->blocks[i]) {
			case BLOCK_STRONG_1_CODE :
			case BLOCK_STRONG_2_CODE :
			case BLOCK_STRONG_3_CODE :
			case BLOCK_EXPLODE_CODE :
			case BLOCK_DEFAULT_CODE :
				level->blocks_left++;
				/* follow through */
			case BLOCK_INVINCIBLE_CODE :
				level->blocks[i] = new_block(rawlevel->blocks[i], i);
				break;
			case BLOCK_NONE_CODE :
				level->blocks[i] = NULL;
				break;
			default :
				g_assert_not_reached();
		}

	}

	level->difficulty = rawlevel->difficulty;
	level->name = g_strdup(rawlevel->name);
	level->author = g_strdup(rawlevel->author);
	level->levelfile_title = g_strdup(rawlevel->levelfile_title);
	return level;
}

/* Make a new block of type at x/y */
static Block *new_block(char type, gint block_no) {
	Block *newblock;
	gint x, y;

	x = block_no % BLOCKS_X;
	y = block_no / BLOCKS_X;
	g_assert(y < BLOCKS_Y);
	newblock = g_malloc(sizeof(Block));

	/* Set the geometry */
	newblock->geometry.x1 = BLOCK_WALL_PADDING + BLOCK_WIDTH * x;
	newblock->geometry.y1 = BLOCK_WALL_PADDING + BLOCK_HEIGHT * y;
	newblock->geometry.x2 = newblock->geometry.x1 + BLOCK_WIDTH;
	newblock->geometry.y2 = newblock->geometry.y1 + BLOCK_HEIGHT;

	g_assert(newblock->geometry.x1 >= BLOCK_WALL_PADDING);
	g_assert(newblock->geometry.y1 >= BLOCK_WALL_PADDING);
	g_assert(newblock->geometry.x2 <= GAME_WIDTH - BLOCK_WALL_PADDING);
	g_assert(newblock->geometry.y2 <= GAME_HEIGHT - BLOCK_WALL_PADDING - BAT_SPACE);

	/* Set block_no */
	newblock->block_no = block_no;

	/* Set everything else */
	switch(type) {
		case BLOCK_STRONG_1_CODE :
			newblock->animation = get_static_animation(
					ANIM_BLOCK_STRONG_1);
			newblock->type = BLOCK_STRONG_1;
			break;
		case BLOCK_STRONG_2_CODE :
			newblock->animation = get_static_animation(
					ANIM_BLOCK_STRONG_2);
			newblock->type = BLOCK_STRONG_2;
			break;
		case BLOCK_STRONG_3_CODE :
			newblock->animation = get_static_animation(
					ANIM_BLOCK_STRONG_3);
			newblock->type = BLOCK_STRONG_3;
			break;
		case BLOCK_DEFAULT_CODE :
			newblock->animation = get_static_animation(
					ANIM_BLOCK_DEFAULT);
			newblock->type = BLOCK_DEFAULT;
			break;
		case BLOCK_INVINCIBLE_CODE :
			newblock->animation = get_static_animation(
					ANIM_BLOCK_INVINCIBLE);
			newblock->type = BLOCK_INVINCIBLE;
			break;
		case BLOCK_EXPLODE_CODE :
			newblock->animation = get_static_animation(
					ANIM_BLOCK_EXPLODE);
			newblock->type = BLOCK_EXPLODE;
			break;
		default :
			g_assert_not_reached();
	}

	/* Add it to the gnome-canvas */
	add_to_canvas((Entity *) newblock);
	
	return newblock;
}

/* Remove a block from the list */
static void remove_block(Block **blocks, Block *block) {
	remove_from_canvas((Entity *) block);
	blocks[block->block_no] = NULL;	
	g_free(block);
}

/* Hit a block */
void hit_block(Game *game, Block *block) {
	gint block_no;

	block_no = block->block_no;

	switch(block->type) {
		case BLOCK_STRONG_1 :
		case BLOCK_STRONG_2 :
		case BLOCK_STRONG_2_DIE :
		case BLOCK_STRONG_3 :
		case BLOCK_STRONG_3_DIE :
			block_strong_hit(game, block);
			ADD_SCORE(game, 20);
			break;
		case BLOCK_STRONG_1_DIE :
		case BLOCK_DEFAULT :
			block_default_hit(game, block);
			ADD_SCORE(game, 50);
			game->level->blocks_left--;
			break;
		case BLOCK_EXPLODE :
			block_explode_hit(game, block);
			ADD_SCORE(game, 100);
			game->level->blocks_left--;
			break;
		case BLOCK_INVINCIBLE :
		case BLOCK_DEAD :
			break;
		default :
			g_assert_not_reached();
	}
}

/* This is here because hit_block got too big for its boots */
static void block_default_hit(Game *game, Block *block) {

	/* Make the block "fade out" */
	remove_from_canvas((Entity *) block);
	block->animation = get_once_animation(ANIM_BLOCK_DEFAULT_DIE);
	block->type = BLOCK_DEAD;
	add_to_canvas((Entity *) block);

	/* Spawn a new powerup */
	new_powerup(game, block->geometry.x1, block->geometry.y2);
	
}

static void block_explode_hit(Game *game, Block *block) {
	Block **nearby;
	int i;
		
        /* Make the block "fade out" */
        remove_from_canvas((Entity *) block);
        block->animation = get_once_animation(ANIM_BLOCK_EXPLODE_DIE);
        block->type = BLOCK_DEAD;
        add_to_canvas((Entity *) block);

        /* Spawn a new powerup */
        new_powerup(game, block->geometry.x1, block->geometry.y2);

	/* Hit the blocks around the exploder block */
	nearby = get_nearby_blocks(game, block);
	for(i = 0; i < 8; i++) {
		if(nearby[i]) {
			hit_block(game, nearby[i]);
			/* MMM, recursion :) */
		}
	}
	g_free(nearby);
}

static void block_strong_hit(Game *game, Block *block) {

	remove_from_canvas((Entity *) block);
	new_powerup(game, block->geometry.x1, block->geometry.y2);

	/* Make the block "fade" into the less strong one */
	switch(block->type) {
		case BLOCK_STRONG_3 :
			block->animation = get_once_animation
				(ANIM_BLOCK_STRONG_3_DIE);
			block->type = BLOCK_STRONG_3_DIE;
			break;
		case BLOCK_STRONG_2 :
		case BLOCK_STRONG_3_DIE :
			block->animation = get_once_animation
				(ANIM_BLOCK_STRONG_2_DIE);
			block->type = BLOCK_STRONG_2_DIE;
			break;
		case BLOCK_STRONG_1 :
		case BLOCK_STRONG_2_DIE :
			block->animation = get_once_animation
				(ANIM_BLOCK_STRONG_1_DIE);
			block->type = BLOCK_STRONG_1_DIE;
			break;
		default :
			g_assert_not_reached();
	}

	add_to_canvas((Entity *) block);
}
	
/* De-allocate the blocks, and the level structure */
void destroy_level(Game *game) {
	int i;

	for(i = 0; i < BLOCKS_X * BLOCKS_Y; i++)
		if(game->level->blocks[i]) 
			remove_block(game->level->blocks,
					game->level->blocks[i]);

	g_free(game->level->name);
	g_free(game->level->author);
	g_free(game->level->levelfile_title);
	g_free(game->level);
	game->level = NULL;
}

/* Check to see if the level is complete */
gboolean check_level_end(Game *game) {
	return(!(game->level->blocks_left));
}

/* Iterate the blocks. Currently, this just animates the dying blocks and
 * kills the ones which have run out of frames. */
void iterate_blocks(Game *game) {
	Block **blocks;
	int i;

	if(game->level) {
		blocks = game->level->blocks;
		for(i = 0; i < BLOCKS_X * BLOCKS_Y; i++) {
			if(blocks[i]) {
				iterate_block(game, blocks[i]);
			}
		}
	}
}

/* Not in iterate_blocks, because it became too deeply nested */
static void iterate_block(Game *game, Block *block) {
	iterate_animation((Entity *) block);

	if((block->type == BLOCK_STRONG_1_DIE
			|| block->type == BLOCK_STRONG_2_DIE
			|| block->type == BLOCK_STRONG_3_DIE
			|| block->type == BLOCK_DEAD)
			&& block->animation.type == ANIM_STATIC) {
		remove_from_canvas((Entity *) block);
		switch(block->type) {
			case BLOCK_STRONG_3_DIE :
				block->type = BLOCK_STRONG_2;
				block->animation = get_static_animation
					(ANIM_BLOCK_STRONG_2);
				break;
			case BLOCK_STRONG_2_DIE :
				block->type = BLOCK_STRONG_1;
				block->animation = get_static_animation
					(ANIM_BLOCK_STRONG_1);
				break;
			case BLOCK_STRONG_1_DIE :
				block->type = BLOCK_DEFAULT;
				block->animation = get_static_animation
					(ANIM_BLOCK_DEFAULT);
				break;
			case BLOCK_DEAD :
				remove_block(game->level->blocks, block);
				block = NULL;
				break;
			default :
				g_assert_not_reached();
		}
		if(block)
			add_to_canvas((Entity *) block);
	}
}

/* Finds the block that an entity occupies. Will return NULL if
 * the entity is outside the range of blocks, there is no block there,
 * or if the block is of type BLOCK_DEAD. Assumes that entity is SMALLER
 * than a block both horizontally and vertically */
Block *find_block_from_position(Game *game, Entity *entity) {
	gint return_val = -1;
	gint x1, x2, y1, y2;
	gint block_no = -1, i;
	Block **blocks;

	if(entity->geometry.x2 > BLOCK_WALL_PADDING
			&& entity->geometry.x1 < GAME_WIDTH - BLOCK_WALL_PADDING
			&& entity->geometry.y2 > BLOCK_WALL_PADDING
			&& entity->geometry.y1 < BLOCK_WALL_PADDING + BLOCKS_Y *
			BLOCK_HEIGHT) {
		blocks = game->level->blocks;
		x1 = entity->geometry.x1 - BLOCK_WALL_PADDING;
		x2 = entity->geometry.x2 - BLOCK_WALL_PADDING;
		y1 = entity->geometry.y1 - BLOCK_WALL_PADDING;
		y2 = entity->geometry.y2 - BLOCK_WALL_PADDING;

		x1 /= BLOCK_WIDTH;
		x2 /= BLOCK_WIDTH;
		y1 /= BLOCK_HEIGHT;
		y2 /= BLOCK_HEIGHT;

		g_assert(x1 < BLOCKS_X);
		if(x2 == BLOCKS_X)
			x2 = BLOCKS_X - 1;
		g_assert(y1 < BLOCKS_Y);
		if(y2 == BLOCKS_Y)
			y2 = BLOCKS_Y - 1;

		for(i = 0; i < 4; i++) {
			switch(i) {
				case 0 :
					block_no = x1 + y1 * BLOCKS_X;
					break;
				case 1 :
					block_no = x2 + y1 * BLOCKS_X;
					break;
				case 2 :
					block_no = x1 + y2 * BLOCKS_X;
					break;
				case 3 :
					block_no = x2 + y2 * BLOCKS_X;
					break;
				default :
					g_assert_not_reached();
			}
			if(block_no >= 0 && block_no < BLOCKS_X * BLOCKS_Y) {
				if(blocks[block_no] && blocks[block_no]->type !=
						BLOCK_DEAD) {
					return_val = block_no;
					break;
				}
			}
		}
	}

	if(return_val != -1)
		return game->level->blocks[block_no];
	else
		return NULL;
}

/* Returns whether the neighbour of 'block_no' on 'side' exists
 * The if statements rely on the fact that if the first condition fails, the
 * other conditions will not be evaluated. I'm not sure, but I think that
 * this is standard for all compilers.
 */
gboolean block_has_neighbour(Game *game, Block *block, Side side) {
	gint block_no;
	Block **blocks;
	gboolean ret = FALSE;
	blocks = game->level->blocks;

	block_no = block->block_no;

	switch(side) {
		case SIDE_TOP :
			if(block_no >= BLOCKS_X && blocks[block_no - BLOCKS_X]
					&& blocks[block_no - BLOCKS_X]->type
					!= BLOCK_DEAD)
				ret = TRUE;
			break;
		case SIDE_LEFT :
			if(block_no >= 1 && blocks[block_no - 1]
					&& blocks[block_no - 1]->type
					!= BLOCK_DEAD)
				ret = TRUE;
			break;
		case SIDE_RIGHT :
			if(block_no < (BLOCKS_X * BLOCKS_Y)
					&& block_no % BLOCKS_X != BLOCKS_X - 1
					&& blocks[block_no + 1]
					&& blocks[block_no + 1]->type
					!= BLOCK_DEAD)
				ret = TRUE;
			break;
		case SIDE_BOTTOM :
			if(block_no < (BLOCKS_X * (BLOCKS_Y - 1))
					&& blocks[block_no + BLOCKS_X]
					&& blocks[block_no + BLOCKS_X]->type
					!= BLOCK_DEAD)
				ret = TRUE;
			break;
		default :
			g_assert_not_reached();
	}

	return ret;
}

/* Destroys a block. Public function for ball.c:iterate_ball_default */
void destroy_block(Game *game, Block *block) {
	if(block->type != BLOCK_DEAD && block->type != BLOCK_INVINCIBLE)
		game->level->blocks_left--;

	remove_block(game->level->blocks, block);
}

/* Returns an array of 8 pointers to the blocks surrounding block. If a
 * pointer is null, then there is no block in that direction. The array
 * starts at north and goes clockwise */
static Block **get_nearby_blocks(Game *game, Block *block) {
	Block **blocks;
	int block_no;
	Block **ret;

	blocks = game->level->blocks;
	ret = g_malloc(sizeof(Block *) * 8);
	block_no = block->block_no;

	memset(ret, 0, sizeof(Block *) * 8);

	/* North */
	if(block_no >= BLOCKS_X) {
		ret[0] = blocks[block_no - BLOCKS_X];
	}

	/* Northeast */
	if(block_no >= BLOCKS_X && block_no % BLOCKS_X != (BLOCKS_X - 1)) {
		ret[1] = blocks[block_no - BLOCKS_X + 1];
	}

	/* East */
	if(block_no < (BLOCKS_TOTAL - 1) && block_no % BLOCKS_X != (BLOCKS_X -1)) {
		ret[2] = blocks[block_no + 1];
	}

	/* Southeast */
	if(block_no <= (BLOCKS_TOTAL - 1 - BLOCKS_X) && block_no % BLOCKS_X != (BLOCKS_X - 1)) {
		ret[3] = blocks[block_no + 1 + BLOCKS_X];
	}

	/* South */
	if(block_no < (BLOCKS_TOTAL - BLOCKS_X)) {
		ret[4] = blocks[block_no + BLOCKS_X];
	}

	/* Southwest */
	if(block_no < (BLOCKS_TOTAL - BLOCKS_X) && (block_no % BLOCKS_X)) {
		ret[5] = blocks[block_no - 1 + BLOCKS_X];
	}

	/* West */
	if(block_no && (block_no % BLOCKS_X)) {
		ret[6] = blocks[block_no - 1];
	}

	/* Northwest */
	if(block_no >= BLOCKS_X && (block_no % BLOCKS_X)) {
		ret[7] = blocks[block_no - BLOCKS_X - 1];
	}

	return ret;
}
