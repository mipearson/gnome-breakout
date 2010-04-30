/*
 * The filenames of the animation pictures. Also see anim.h
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

/*
 * Every image is in .png format, and is suffixed by .frameno.png
 * eg, "foo" would have files to the tune of "foo.0.png" up to "foo.99.png".
 * 'animations' which are only still images should not be named simply
 * "foo.png", rather "foo.0.png".
 */
char *animlocations[] = {
	"block.default",             /* ANIM_BLOCK_DEFAULT */
	"block.strong.1",            /* ANIM_BLOCK_STRONG_1 */
	"block.strong.1.die",        /* ANIM_BLOCK_STRONG_1_DIE */
	"block.strong.2",            /* ANIM_BLOCK_STRONG_2 */
	"block.strong.2.die",        /* ANIM_BLOCK_STRONG_2_DIE */
	"block.strong.3",            /* ANIM_BLOCK_STRONG_3 */
	"block.strong.3.die",        /* ANIM_BLOCK_STRONG_3_DIE */
	"block.invincible",          /* ANIM_BLOCK_INVINCIBLE */
	"block.default.die",         /* ANIM_BLOCK_DEFAULT_DIE */
	"ball.default",              /* ANIM_BALL_DEFAULT */
	"bat.default",               /* ANIM_BAT_DEFAULT */
	"powerup.score500",          /* ANIM_POWERUP_SCORE500 */
	"bat.laser",                 /* ANIM_BAT_LASER */
	"laser",                     /* ANIM_LASER */
	"powerup.laser",             /* ANIM_POWERUP_LASER */
	"powerup.newlife",           /* ANIM_POWERUP_NEWLIFE */
	"powerup.newball",           /* ANIM_POWERUP_NEWBALL */
	"powerup.nextlevel",         /* ANIM_POWERUP_NEXTLEVEL */
	"powerup.slow",              /* ANIM_POWERUP_SLOW */
	"powerup.widebat",           /* ANIM_POWERUP_WIDEBAT */
	"bat.wide",                  /* ANIM_BAT_WIDE */
	"block.explode",             /* ANIM_BLOCK_EXPLODE */
	"block.explode.die",         /* ANIM_BLOCK_EXPLODE_DIE */
	NULL
};
