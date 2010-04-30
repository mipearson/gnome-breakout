/*
 * Handles loading and storing the flags, and also making decisions
 * based on the flags
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include "breakout.h"
#include "flags.h"
#include "util.h"

/* Configuration Defaults. Be careful here, because compute_flags falls
 * back to these when something goes wrong. If these values are wrong,
 * weird things may happen. */
#define DEFAULT_DIFFICULTY DIFFICULTY_MEDIUM
#define DEFAULT_MOUSE_CONTROL "true"
#define DEFAULT_KEYBOARD_CONTROL "false"
#define DEFAULT_BAT_SPEED 15 
#define DEFAULT_PAUSE_ON_FOCUS "false"
#define DEFAULT_PAUSE_ON_POINTER "true"
#define DEFAULT_PAUSE_ON_PREF "true"
#define DEFAULT_HIDE_POINTER "true"
#define DEFAULT_LEFT_KEY GDK_Left
#define DEFAULT_RIGHT_KEY GDK_Right
#define DEFAULT_FIRE1_KEY GDK_z
#define DEFAULT_FIRE2_KEY GDK_x
#define DEFAULT_BOUNCE_ENTROPY 0
#define DEFAULT_LEVEL_FILES (LEVELDIR "/alcaron.gbl;" LEVELDIR "/mdutour.gbl;" LEVELDIR "/mmack.gbl")

/* Difficulty modifiers */
#define EASY_SCORE_MODIFIER 0.5 
#define EASY_BALL_INITIAL_SPEED 4 
#define EASY_BALL_SPEED_INCREMENT 0.10
#define EASY_BALL_MAX_SPEED 8

#define MEDIUM_SCORE_MODIFIER 1.0
#define MEDIUM_BALL_INITIAL_SPEED 7
#define MEDIUM_BALL_SPEED_INCREMENT 0.25
#define MEDIUM_BALL_MAX_SPEED 12

#define HARD_SCORE_MODIFIER 1.5
#define HARD_BALL_INITIAL_SPEED 9
#define HARD_BALL_SPEED_INCREMENT 0.25
#define HARD_BALL_MAX_SPEED 17

/* Internal Functions */
GList *unpack_string_list(gchar *s);
gchar *pack_string_list(GList *s);

/* Loads the flags, and computes certain difficulty values */
Flags *load_flags(void) {
	Flags *flags;
	gchar *tmp;

	flags = g_malloc(sizeof(Flags));

	gnome_config_push_prefix("/" PACKAGE "/");

	flags->mouse_control = gnome_config_get_bool(
			"control/mouse_control=" DEFAULT_MOUSE_CONTROL);
	flags->keyboard_control = gnome_config_get_bool(
			"control/keyboard_control=" DEFAULT_KEYBOARD_CONTROL);
	flags->pause_on_focus = gnome_config_get_bool(
			"game/pause_on_focus=" DEFAULT_PAUSE_ON_FOCUS);
	flags->pause_on_pointer = gnome_config_get_bool(
			"game/pause_on_pointer=" DEFAULT_PAUSE_ON_POINTER);
	flags->pause_on_pref = gnome_config_get_bool(
			"game/pause_on_pref=" DEFAULT_PAUSE_ON_PREF);
	flags->hide_pointer = gnome_config_get_bool(
			"game/hide_pointer=" DEFAULT_HIDE_POINTER);

	tmp = g_strdup_printf("game/bounce_entropy=%d", DEFAULT_BOUNCE_ENTROPY);
	flags->bounce_entropy = gnome_config_get_int(tmp);
	g_free(tmp);

	tmp = g_strdup_printf("control/bat_speed=%d", DEFAULT_BAT_SPEED);
	flags->bat_speed = gnome_config_get_int(tmp);
	g_free(tmp);

	tmp = g_strdup_printf("game/difficulty=%d", DEFAULT_DIFFICULTY);
	flags->next_game_difficulty = gnome_config_get_int(tmp);
	flags->difficulty = flags->next_game_difficulty;
	g_free(tmp);

	tmp = g_strdup_printf("keys/left_key=%d", DEFAULT_LEFT_KEY);
	flags->left_key = gnome_config_get_int(tmp);
	g_free(tmp);

	tmp = g_strdup_printf("keys/right_key=%d", DEFAULT_RIGHT_KEY);
	flags->right_key = gnome_config_get_int(tmp);
	g_free(tmp);

	tmp = g_strdup_printf("keys/fire1_key=%d", DEFAULT_FIRE1_KEY);
	flags->fire1_key = gnome_config_get_int(tmp);
	g_free(tmp);

	tmp = g_strdup_printf("keys/fire2_key=%d", DEFAULT_FIRE2_KEY);
	flags->fire2_key = gnome_config_get_int(tmp);
	g_free(tmp);

	tmp = g_strdup_printf("game/level_files=%s", DEFAULT_LEVEL_FILES);
	flags->level_files = unpack_string_list(gnome_config_get_string(tmp));
	g_free(tmp);

	gnome_config_pop_prefix();
	compute_flags(flags);

	return flags;
}

/* Computes the difficulty modifiers of Flags, and makes various config sanity 
 * checks */
void compute_flags(Flags *flags) {
	gboolean resetcontrol = FALSE;

	 /* Difficulty sanity checks */
	if(flags->next_game_difficulty != DIFFICULTY_EASY
			&& flags->next_game_difficulty != DIFFICULTY_MEDIUM
			&& flags->next_game_difficulty != DIFFICULTY_HARD) {
		gb_warning(_("Difficulty in config files is an incorrect value, resetting to defaults"));
		flags->next_game_difficulty = DEFAULT_DIFFICULTY;
	}

	/* Difficulty computation */
	switch(flags->difficulty) {
		case DIFFICULTY_EASY :
			flags->score_modifier = EASY_SCORE_MODIFIER;
			flags->ball_initial_speed = EASY_BALL_INITIAL_SPEED;
			flags->ball_speed_increment = EASY_BALL_SPEED_INCREMENT;
			flags->ball_max_speed = EASY_BALL_MAX_SPEED;
			break;
		case DIFFICULTY_MEDIUM :
			flags->score_modifier = MEDIUM_SCORE_MODIFIER;
			flags->ball_initial_speed = MEDIUM_BALL_INITIAL_SPEED;
			flags->ball_speed_increment = MEDIUM_BALL_SPEED_INCREMENT;
			flags->ball_max_speed = MEDIUM_BALL_MAX_SPEED;
			break;
		case DIFFICULTY_HARD :
			flags->score_modifier = HARD_SCORE_MODIFIER;
			flags->ball_initial_speed = HARD_BALL_INITIAL_SPEED;
			flags->ball_speed_increment = HARD_BALL_SPEED_INCREMENT;
			flags->ball_max_speed = HARD_BALL_MAX_SPEED;
			break;
		default :
			g_assert_not_reached();
	}

	/* Control sanity checks */
	if(flags->keyboard_control && flags->mouse_control) {
		gb_warning(_("Both keyboard control and mouse control are TRUE in user preferences, resetting to defaults"));
		resetcontrol = TRUE;
	}

	if(!(flags->keyboard_control || flags->mouse_control)) {
		gb_warning(_("Neither keyboard control or mouse control are TRUE in user preferences, resetting to defaults"));
		resetcontrol = TRUE;
	}

	if(resetcontrol) {
		if(!strcmp(DEFAULT_KEYBOARD_CONTROL, "true"))
			flags->keyboard_control = TRUE;
		else
			flags->keyboard_control = FALSE;
		if(!strcmp(DEFAULT_MOUSE_CONTROL, "true"))
			flags->mouse_control = TRUE;
		else
			flags->keyboard_control = FALSE;
		/* Check to see if we've messed the defaults */
		g_assert(flags->keyboard_control || flags->mouse_control);
		g_assert(!(flags->keyboard_control && flags->mouse_control));
	}

	/* Bat speed sanity checks */
	if(flags->bat_speed < MIN_BATSPEED)  {
		gb_warning(_("Bat speed is lower than allowed range, setting to lowest"));
		flags->bat_speed = MIN_BATSPEED;
	}
	if(flags->bat_speed > MAX_BATSPEED) {
		gb_warning(_("Bat speed is higher than allowed range, setting to highest"));
		flags->bat_speed = MAX_BATSPEED;
	}

	/* Bounce entropy sanity checks */
	if(flags->bounce_entropy < MIN_BOUNCE_ENTROPY)  {
		gb_warning(_("Bounce entropy is lower than allowed range, setting to lowest"));
		flags->bounce_entropy = MIN_BOUNCE_ENTROPY;
	}
	if(flags->bounce_entropy > MAX_BOUNCE_ENTROPY) {
		gb_warning(_("Bounce entropy is higher than allowed range, setting to highest"));
		flags->bounce_entropy = MAX_BOUNCE_ENTROPY;
	}
}

/* Saves the flags */
void save_flags(Flags *flags) {
	gchar *tmp;

	gnome_config_push_prefix("/" PACKAGE "/");

	gnome_config_set_int("game/difficulty", flags->next_game_difficulty);
	gnome_config_set_bool("game/pause_on_focus", flags->pause_on_focus);
	gnome_config_set_bool("game/pause_on_pointer", flags->pause_on_pointer);
	gnome_config_set_bool("game/pause_on_pref", flags->pause_on_pref);
	gnome_config_set_bool("game/hide_pointer", flags->hide_pointer);
	gnome_config_set_int("game/bounce_entropy", flags->bounce_entropy);
	gnome_config_set_bool("control/mouse_control", flags->mouse_control);
	gnome_config_set_bool("control/keyboard_control",
			flags->keyboard_control);
	gnome_config_set_int("control/bat_speed", flags->bat_speed);
	gnome_config_set_int("keys/left_key", flags->left_key);
	gnome_config_set_int("keys/right_key", flags->right_key);
	gnome_config_set_int("keys/fire1_key", flags->fire1_key);
	gnome_config_set_int("keys/fire2_key", flags->fire2_key);
	tmp = pack_string_list(flags->level_files);
	gnome_config_set_string("game/level_files", tmp);
	g_free(tmp);	

	gnome_config_pop_prefix();
	gnome_config_sync();
}

/* Takes a string, splitting it by the ; character, returning a list of the
 * split values */
GList *unpack_string_list(gchar *s) {
	GList *ret = NULL;
	gchar *s_p, *s_new;

	if(*s) {
		while((s_p = (gchar *) strsep(&s, ";"))) {
			if(*s_p) {
				s_new = g_strdup(s_p);
				ret = g_list_prepend(ret, s_new);
			}
		}
	}

	return ret;
}

/* Takes a list of strings and generates a big string, seperating each entry
 * with a ; */
gchar *pack_string_list(GList *l) {
	GList *l_p;
	gchar *ret;

	ret = g_malloc(1024 * sizeof(gchar));
	*ret = '\0';
	for(l_p = l; l_p; l_p = g_list_next(l_p)) {
		strncat(ret, (char *) l_p->data, 1024);
		strncat(ret, ";", 1024);
	}

	return ret;
}

/* Allocs and copies a Flags structure. */
Flags *copy_flags(Flags *flags) {
	Flags *newflags;
	GList *curr;

	newflags = g_malloc(sizeof(Flags));
	
	/* Most values can simply be memcpy'ed.... */
	memcpy(newflags, flags, sizeof(Flags));

	/* ... except for the LevelFiles list */
	newflags->level_files = NULL;
	for(curr = flags->level_files; curr; curr = g_list_next(curr)) {
		newflags->level_files = g_list_prepend(newflags->level_files, g_strdup((gchar *) curr->data));
	}

	return newflags;
}

/* Destroys and frees a Flags structure */
void destroy_flags(Flags *flags) {
	GList *curr;

	for(curr = flags->level_files; curr; curr = g_list_next(curr)) {
		g_free(curr->data);
		curr->data = NULL;
	}

	g_list_free(flags->level_files);
	g_free(flags);
}

/* Adds a levelfile to the Flags structure */
void add_flags_levelfile(Flags *flags, gchar *filename) {
	g_assert(flags && filename);
	flags->level_files = g_list_prepend(flags->level_files, g_strdup(filename));
}

/* Removes a levelfile from the Flags structure. Assumes that it's there in the
 * first place */
void remove_flags_levelfile(Flags *flags, gchar *filename) {
	GList *curr;
	g_assert(flags && filename);

	for(curr = flags->level_files; curr && strcmp((gchar *) curr->data, filename); curr = g_list_next(curr));

	g_assert(curr && curr->data);
	g_free(curr->data);
	flags->level_files = g_list_remove(flags->level_files, curr->data);
}
