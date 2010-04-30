/*
 * gnome-breakout global types and defines
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include <gdk/gdk.h>
#include <gnome.h>
#include <libgnomecanvas/libgnomecanvas.h>

/*
 * Dimensions.
 */
/* The width of the playing field */
#define GAME_WIDTH (BLOCKS_X * BLOCK_WIDTH + BLOCK_WALL_PADDING * 2)
/* The height of the playing field */
#define GAME_HEIGHT (BLOCKS_Y * BLOCK_HEIGHT + BLOCK_WALL_PADDING * 2 + BAT_SPACE)
#define BLOCKS_X   10 /* The number of blocks along the X plane */
#define BLOCKS_Y   15 /* The number of blocks along the Y plane */
#define BLOCKS_TOTAL (BLOCKS_X * BLOCKS_Y)
#define BLOCK_WALL_PADDING 20 /* The distance between the wall and the blocks */
#define BLOCK_WIDTH 40 /* The width of the blocks */
#define BLOCK_HEIGHT 20 /* The height of the blocks */
#define BAT_SPACE 100 /* The distance between the lowest block and the bottom wall*/
#define BAT_WIDTH 75 /* The width of the bat */
#define BAT_WIDE_WIDTH 100 /* The width of a "wide" bat */
#define BAT_HEIGHT 10 /* The height of the bat */
#define POWERUP_WIDTH 20 /* The width of the powerup */
#define POWERUP_HEIGHT 20 /* The height of the powerup */
#define BALL_WIDTH 10 /* Are we getting the idea yet? */
#define BALL_HEIGHT 10 /* Is this comment necessary? No. It isn't. */

/*
 * Where the object appears on the screen, and how big it is. Mostly used for
 * drawing the object and collision detection. x1/y1 is the top left corner,
 * and x2/y2 is the bottom right corner.
 */
typedef struct {
	gint x1;
	gint y1;
	gint x2;
	gint y2;
} Geometry;

/* 
 * Info about how to draw the object. STATIC type images aren't animated,
 * ANIM_LOOP images loop their animation, and ANIM_ONCE images iterate their
 * animation once, and become STATIC. frame_no counts up the number of
 * remaining frames. pixmaps are the animation frames. 
 */
typedef enum { ANIM_STATIC, ANIM_LOOP, ANIM_ONCE } AnimType; 
typedef struct {
	gint frame_no;
	gint num_frames;
	GdkPixbuf **pixmaps;
	GnomeCanvasItem *canvas_item;
	AnimType type;
} Animation;

/*
 * In OO terms, this would loosely be termed a superclass. Most of the object
 * structures inherit from it.
 */
typedef struct {
	Geometry geometry;
	Animation animation;
} Entity;

/*
 * Details about a ball. 
 * direction is in radians. The pseudo values are for handling fine
 * direction control. Airtime is how many frames the ball has been in the air.
 */
typedef enum { BALL_DEFAULT, BALL_STUCK } BallType;
typedef struct {
	Geometry geometry;
	Animation animation;
	gdouble pseudo_x1;
	gdouble pseudo_y1;
	gdouble speed;
	gdouble direction;
	gint airtime;
	BallType type; 
} Ball;

/*
 * Details about a block.
 */
typedef enum { BLOCK_DEFAULT, BLOCK_INVINCIBLE, BLOCK_DEAD, BLOCK_STRONG_1,
	BLOCK_STRONG_2, BLOCK_STRONG_3, BLOCK_STRONG_1_DIE,
	BLOCK_STRONG_2_DIE, BLOCK_STRONG_3_DIE, BLOCK_EXPLODE
} BlockType;

typedef struct {
	Geometry geometry;
	Animation animation;
	BlockType type;
	gint block_no;
} Block;

/*
 * Details about a bat.
 */
typedef enum { BAT_DEFAULT, BAT_LASER, BAT_WIDE } BatType;
typedef struct {
	Geometry geometry;
	Animation animation;
	gint width;
	GList *children;
	gint num_lasers_allowed;
	gint num_lasers;
	BatType type;
} Bat;

/*
 * Details about a powerup.
 */
typedef enum { POWER_SCORE500, POWER_LASER, POWER_NEWLIFE,
	POWER_NEWBALL, POWER_NEXTLEVEL, POWER_SLOW, POWER_WIDEBAT
} PowerupType;

typedef struct {
	Geometry geometry;
	Animation animation;
	PowerupType type;
} Powerup;

/*
 * Flags and data directly based on user preferences
 */
typedef enum { DIFFICULTY_EASY = 1,
	DIFFICULTY_MEDIUM = 0,
	DIFFICULTY_HARD = 2
} Difficulty;

#define ADD_SCORE(game, _score) game->score += _score * game->flags->score_modifier

typedef struct {
	/* User set values */
	Difficulty difficulty;
	Difficulty next_game_difficulty;
	gboolean mouse_control;
	gboolean keyboard_control;
	gint bat_speed;
	gboolean slippery_bat;
	gint left_key;
	gint right_key;
	gint fire1_key;
	gint fire2_key;
	gboolean hide_pointer;
	gboolean pause_on_focus;
	gboolean pause_on_pointer;
	gboolean pause_on_pref;
	gint bounce_entropy;
	GList *level_files;

	/* Computed values */
	gdouble score_modifier;
	gdouble ball_initial_speed;
	gdouble ball_speed_increment;
	gdouble ball_max_speed;
} Flags;

/*
 * The current level
 */
typedef struct {
	Block *blocks[BLOCKS_TOTAL];
	gint difficulty;
	gint number;
	gchar *name;
	gchar *author;
	gchar *levelfile_title;
	gint blocks_left;
} Level;

/*
 * Raw level data, pre block generation.
 */
typedef struct {
	gchar blocks[BLOCKS_TOTAL];
	gint difficulty;
	gchar *name;
	gchar *author;
	gchar *levelfile_title;
} RawLevel;

/*
 * Information about the current game.
 */
typedef enum { STATE_STOPPED = 0, STATE_RUNNING, STATE_PAUSED } GameState; 
typedef struct {
	GameState state;
	gboolean paused_explicit;
	gint32 score;
	gint32 last_newlife_score;
	gint lives;
	gint level_no;
	Flags *flags;

	/* Entities */
	GList *balls;
	GList *powerups;
	Bat *bat;
	Level *level;

	/* Control Input */
	gint keyboard_move;
	gint mouse_move;
	gboolean fire1_pressed;
	gboolean fire2_pressed;

	/* Pause Levels bitmask */
	gint32 pause_state;

	/* Pending Events */
	gboolean powerup_next_level;
} Game;

typedef enum { SIDE_NONE, SIDE_TOP, SIDE_BOTTOM, SIDE_LEFT, SIDE_RIGHT, SIDE_DIAGONAL } Side;

/*
 * Bitmask for pause states
 */
typedef enum {
        PAUSE_MENU =    0x00000001,
        PAUSE_FOCUS =   0x00000002,
        PAUSE_POINTER = 0x00000004,
        PAUSE_PREF =    0x00000008,
        PAUSE_FORCE =   0xffffffff
} PauseType;

/*
 * What triggered the end of the game
 */
typedef enum { ENDGAME_WIN, ENDGAME_LOSE, ENDGAME_MENU } EndGameStatus;
