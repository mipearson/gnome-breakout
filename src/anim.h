/*
 * Functions to create and manipulate animation structures
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

/* Animation IDs. See also animdata.h */
#define ANIM_BLOCK_DEFAULT 0
#define ANIM_BLOCK_STRONG_1 1
#define ANIM_BLOCK_STRONG_1_DIE 2
#define ANIM_BLOCK_STRONG_2 3
#define ANIM_BLOCK_STRONG_2_DIE 4
#define ANIM_BLOCK_STRONG_3 5 
#define ANIM_BLOCK_STRONG_3_DIE 6 
#define ANIM_BLOCK_INVINCIBLE 7
#define ANIM_BLOCK_DEFAULT_DIE 8
#define ANIM_BALL_DEFAULT 9
#define ANIM_BAT_DEFAULT 10
#define ANIM_POWERUP_SCORE500 11
#define ANIM_BAT_LASER 12
#define ANIM_LASER 13
#define ANIM_POWERUP_LASER 14
#define ANIM_POWERUP_NEWLIFE 15
#define ANIM_POWERUP_NEWBALL 16
#define ANIM_POWERUP_NEXTLEVEL 17
#define ANIM_POWERUP_SLOW 18
#define ANIM_POWERUP_WIDEBAT 19
#define ANIM_BAT_WIDE 20
#define ANIM_BLOCK_EXPLODE 21
#define ANIM_BLOCK_EXPLODE_DIE 22

void init_animations(void);
Animation get_animation(gint id);
Animation get_static_animation(gint id);
Animation get_once_animation(gint id);
Animation get_loop_animation(gint id);
void iterate_animation(Entity *entity);
