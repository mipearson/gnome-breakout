/*
 * Functions for loading and parsing custom level files. leveldata.c keeps an
 * internal repository of levels, and which levels belong to which files.
 * Levels should not be added or removed while the game is running.
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is license under the GNU General Public License. See the file
 * "COPYING" for more details
 */

#include "breakout.h"
#include "gui.h"
#include "leveldata.h"
#include <errno.h>
#include <string.h>

/* Internal Contants */
#define DEFAULT_NAME "No Name"
#define DEFAULT_AUTHOR "No Author"
#define DEFAULT_DIFFICULTY 0
#define VALID_STRING "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[];',./!@#$^&*()-=_+`~1234567890 "
#define VALID_INT "1234567890"
#define VALID_WHITESPACE " \n\t"

/* Internal Data Structures */
typedef struct {
	gchar *filename;
	gchar *title;
	GList *levels; /* Contains RawLevel structures */
} LevelFile;

/* Internal Functions */
static void regenerate_level_list(void);
static void add_level_to_levels_list(RawLevel *level);
static void free_levelfile(LevelFile *levelfile);
static void free_rawlevel(RawLevel *level);
static RawLevel *read_level(FILE *fp, gchar *default_name, gchar *default_author, gint default_difficulty, gchar *filename, gint *lineno);
static RawLevel *new_rawlevel(gchar **blocks, gint difficulty, gchar *name, gchar *author, gchar *levelfile_title);
static LevelFile *new_levelfile(gchar *filename, gchar *title);
static LevelFile *load_levelfile(gchar *filename);
static LevelFile *read_levelfile(FILE *fp, gchar *filename);
static gchar *read_levelblocks(FILE *fp, gchar *filename, gint *lineno);
static gchar *sep_string(gchar *line, gchar *tag, gchar *filename, gint lineno);
static gint sep_uint(gchar *line, gchar *tag, gchar *filename, gint lineno);
static void crop_by_whitespace(gchar *string);
static LevelFile *find_levelfile(gchar *filename, gchar *title);
static gchar *sep_by_tag(gchar *line, gchar *tag, gchar *filename, gint lineno);
static gboolean check_valid_chars(gchar *string, gchar *valid, gchar *filename, gint lineno);
static gint verify_raw_data(gint *data, size_t n, gint max);

/* Internal Variables */
static GList *levelfiles = NULL; /* Contains LevelFile structures */
static GList *levels = NULL; /* Contains RawLevel structures */
static gint num_levels = 0;

/* Functions */

/* Public function for adding a levelfile to leveldata.c's internal structures.
 * Returns the levelfile's title on success, NULL otherwise. 
 * This function, and functions that it calls will issue GUI warnings */
gchar *leveldata_add(char *filename) {
	LevelFile *levelfile;

	/* See if we already have this file */
	if(find_levelfile(filename, NULL)) {
		gui_warning(_("Attempt to add a levelfile that we already have: %s"), filename);
		return FALSE;
	}
		
	levelfile = load_levelfile(filename);

	/* If the load succeded, add it to the levelfile list and regenerate
	 * the levels list */
	if(levelfile) {
		levelfiles = g_list_prepend(levelfiles, levelfile);
		regenerate_level_list();
		return levelfile->title;
	} else {
		return NULL;
	}
}
		
/* Removes a levelfile from leveldata.c's internal structures. Returns the
 * filename of the removed levelfile, so that it can be removed from the flags
 * levelfile list. The filename should be freed by you. */
gchar * leveldata_remove(gchar *title) {
	LevelFile *levelfile;
	gchar *filename = NULL;

	levelfile = find_levelfile(NULL, title);
	if(levelfile) {
		filename = g_strdup(levelfile->filename);
		free_levelfile(levelfile);
		levelfiles = g_list_remove(levelfiles, levelfile);
		regenerate_level_list();
	} else {
		g_assert_not_reached();
	}

	return filename;
}

/* Returns the number of levels that we have in our repository */
gint leveldata_num_levels(void) {
	return num_levels;
}

/* Frees and regenerates the sequential level list from the levelfiles list.
 * Also updates num_levels with the number of levels that we have */
static void regenerate_level_list(void) {
	GList *currlf, *currl;
	LevelFile *levelfile;

	if(levels) {
		g_list_free(levels);
		levels = NULL;
	} 
	num_levels = 0;
		
	for(currlf = levelfiles; currlf; currlf = g_list_next(currlf)) {
		levelfile = (LevelFile *) currlf->data;
		for(currl = levelfile->levels; currl; currl = g_list_next(currl)) {
			add_level_to_levels_list((RawLevel *) currl->data);
			num_levels++;
		}
	}
}

/* Adds a RawLevel to the levels list. The levels list is sorted by precedence
 * of difficulty, then alphabetically by level name */
static void add_level_to_levels_list(RawLevel *level) {
	GList *curr;
	RawLevel *currl;
	gint stop = 0;
	gint i;

	for(i = 0, curr = levels; curr && !stop; curr = g_list_next(curr)) {
		currl = (RawLevel *) curr->data;
		if(currl->difficulty == level->difficulty) {
			if(strcmp(currl->name, level->name) >= 0) {
				stop = 1;
			}
		} else if(currl->difficulty > level->difficulty) {
			stop = 1;
		}
		if(!stop) {
			i++;
		}
	}

	levels = g_list_insert(levels, level, i);
}

/* Deallocates a levelfile structure, also destroying its children levels */
static void free_levelfile(LevelFile *levelfile) {
	GList *curr;
	if(levelfile->filename)
		g_free(levelfile->filename);
	if(levelfile->title)
		g_free(levelfile->title);

	if(levelfile->levels) {
		for(curr = levelfile->levels; curr; curr = g_list_next(curr)) {
			free_rawlevel((RawLevel *) curr->data);
		}

		g_list_free(levelfile->levels);
	}

	g_free(levelfile);
}

/* Deallocates a rawlevel structure */
static void free_rawlevel(RawLevel *level) {
	if(level->name)
		g_free(level->name);
	if(level->author)
		g_free(level->author);
	if(level->levelfile_title)
		g_free(level->levelfile_title);

	g_free(level);
}

/* Generates a new levelfile structure. Duplicates and assigns filename/title
 * if provided */
static LevelFile *new_levelfile(gchar *filename, gchar *title) {
	LevelFile *new;

	new = g_malloc(sizeof(LevelFile));

	new->levels = NULL;
	if(filename)
		new->filename = g_strdup(filename);
	else
		new->filename = NULL;

	if(title)
		new->filename = g_strdup(title);
	else
		new->title = NULL;
	
	return new;
}

/* Generates a new rawlevel structure. Duplicates and assigns fields if they
 * are provided */
static RawLevel *new_rawlevel(gchar **blocks, gint difficulty, gchar *name, gchar *author, gchar *levelfile_title) {
	RawLevel *new;

	new = g_malloc(sizeof(RawLevel));
	memset(new, 0, sizeof(RawLevel));

	if(blocks)
		memcpy(new->blocks, blocks, sizeof(gchar) * BLOCKS_TOTAL);
	if(difficulty)
		new->difficulty = difficulty;
	if(name)
		new->name = g_strdup(name);
	if(author)
		new->author = g_strdup(author);
	if(levelfile_title)
		new->levelfile_title = g_strdup(levelfile_title);

	return new;
}

/* Attempts to load a levelfile. Returns 0 on failure */
static LevelFile *load_levelfile(gchar *filename) {
	FILE *fp;
	LevelFile *ret;

	fp = fopen(filename, "r");

	if(!fp) {
		gui_warning(_("Cannot open levelfile %s, discarding: %s"), filename, strerror(errno));
		return 0;
	}

	ret = read_levelfile(fp, filename);

	fclose(fp);
	return ret;
}

/* Reads an already opened levelfile. Returns 0 on an error */
static LevelFile *read_levelfile(FILE *fp, gchar *filename) {
	LevelFile *ret;
	GList *curr;
	gint lineno = 0 , ret_zero = FALSE;
	RawLevel *level;
	gchar buffer[1024];
	gchar *default_author = NULL;
	gchar *default_name = NULL;
	gint default_difficulty = -1;

	ret = new_levelfile(filename, NULL);
	while(!ret_zero && fgets(buffer, 1024, fp)) {
		lineno++;
		crop_by_whitespace(buffer);
		if(!*buffer || *buffer == '#') {
			continue;
		}
		ret_zero = FALSE;

		if(!strncmp(buffer, "GLOBAL_AUTHOR", strlen("GLOBAL_AUTHOR"))) {
			default_author = sep_string(buffer, "GLOBAL_AUTHOR", filename, lineno);
			if(!default_author) {
				ret_zero = TRUE;
			}
		} else if (!strncmp(buffer, "GLOBAL_NAME", strlen("GLOBAL_NAME"))) {
			default_name = sep_string(buffer, "GLOBAL_NAME", filename, lineno);
			if(!default_name) {
				ret_zero = TRUE;
			}
		} else if (!strncmp(buffer, "GLOBAL_DIFFICULTY", strlen("GLOBAL_DIFFICULTY"))) {
			default_difficulty = sep_uint(buffer, "GLOBAL_DIFFICULTY", filename, lineno);
			if(default_difficulty == -1) {
				ret_zero = TRUE;
			}
		} else if (!strcmp(buffer, "BEGIN_LEVEL")) {
			level = read_level(fp, default_name, default_author, default_difficulty, filename, &lineno);
			if(level) {
				ret->levels = g_list_prepend(ret->levels, level);
			} else {
				ret_zero = TRUE;
			}
		} else if (!strncmp(buffer, "TITLE ", strlen("TITLE "))) {
			if(ret->title) {
				g_free(ret->title);
			}

			ret->title = sep_string(buffer, "TITLE", filename, lineno);
			if(!ret->title) {
				ret_zero = TRUE;
			}
		} else {
			gui_warning(_("Unrecognized or incorrectly positioned directive '%s' on line %d of %s"), buffer, lineno, filename);
			ret_zero = TRUE;
		}
	}

	/* Check any weird IO errors */
	if(!ret_zero && !feof(fp)) {
		gui_warning(_("Error while parsing %s: %s"), filename, strerror(errno));
		ret_zero = TRUE;
	}

	if(!ret_zero) {
		/* Syntax tests passed */

		/* Check that we have a title */
		if(!ret_zero && !ret->title)
			ret->title = g_strdup(filename);

		/* Set the "title" entry in each rawlevel */
		for(curr = ret->levels; curr; curr = g_list_next(curr)) {
			level = (RawLevel *) curr->data;
			level->levelfile_title = g_strdup(ret->title);
		}
	} else {
		/* Syntax tests failed */

		/* Free the junk */
		free_levelfile(ret);
		ret = 0;
	}

		
	/* Cleanup our defaults*/
	if(default_name) {
		g_free(default_name);
	}
	if(default_author) {
		g_free(default_author);
	}

	return ret;
}

/* Reads a level section of a levelfile, until it hits an "END_LEVEL". Returns
 * the RawLevel on success, NULL otherwise */
static RawLevel *read_level(FILE *fp, gchar *default_name, gchar *default_author, gint default_difficulty, gchar *filename, gint *lineno) {
	RawLevel *ret;
	char buffer[1024];
	gchar *blocks;
	gint ret_zero = FALSE, end_level = 0;

	ret = new_rawlevel(NULL, -1, NULL, NULL, NULL);
	while(!ret_zero && !end_level && fgets(buffer, 1024, fp)) {
		ret_zero = FALSE;
		crop_by_whitespace(buffer);
		(*lineno)++;
		if(!*buffer || *buffer == '#') {
			continue;
		}

		if(!strcmp(buffer, "END_LEVEL")) {
			end_level = 1;
		} else if (!strcmp(buffer, "BEGIN_DATA")) {
			blocks = read_levelblocks(fp, filename, lineno);
			if(blocks) {
				memcpy(ret->blocks, blocks, sizeof(gchar) * BLOCKS_TOTAL);
			} else {
				ret_zero = TRUE;
			}
		} else if(!strncmp(buffer, "AUTHOR", strlen("AUTHOR"))) {
			ret->author = sep_string(buffer, "AUTHOR", filename, *lineno);
			if(!ret->author) {
				ret_zero = TRUE;
			}
		} else if(!strncmp(buffer, "NAME", strlen("NAME"))) {
			ret->name = sep_string(buffer, "NAME", filename, *lineno);
			if(!ret->name) {
				ret_zero = TRUE;
			}
		} else if(!strncmp(buffer, "DIFFICULTY", strlen("DIFFICULTY"))) {
			ret->difficulty = sep_uint(buffer, "DIFFICULTY", filename, *lineno);
			if(ret->difficulty == -1) {
				ret_zero = TRUE;
			}
		} else {
			gui_warning(_("Unrecognized or incorrectly positioned directive '%s' on line %d of %s"), buffer, *lineno, filename);
			ret_zero = TRUE;
		}
	}

	if(!ret_zero && !end_level) {
		if(feof(fp)) {
			gui_warning(_("Unexpected EOF while parsing level in %s"), filename);
		} else {
			gui_warning(_("Error while parsing %s: %s"), filename, strerror(errno));
		}
		ret_zero = TRUE;
	}

	/* Assign defaults if necessary */
	if(ret_zero) {
		free_rawlevel(ret);
		ret = 0;
	} else {
		if(ret->difficulty == -1) {
			if(default_difficulty == -1) {
				ret->difficulty = DEFAULT_DIFFICULTY;
			} else {
				ret->difficulty = default_difficulty;
			}
		}
		if(!ret->name) {
			if(!default_name) {
				ret->name = g_strdup(DEFAULT_NAME);
			} else {
				ret->name = g_strdup(default_name);
			}
		}
		if(!ret->author) {
			if(!default_author) {
				ret->author = g_strdup(DEFAULT_AUTHOR);
			} else {
				ret->author = g_strdup(default_author);
			}
		}
	}		

	return ret;
}

/* Reads the leveldata section of a level in a levelfile until it hits the
 * END_DATA tag. Returns blocks on success, NULL otherwise */
static gchar *read_levelblocks(FILE *fp, gchar *filename, gint *lineno) {
	static gchar ret[BLOCKS_TOTAL];
	gint r[BLOCKS_X];
	gchar buffer[1024];
	gint i, ret_zero = FALSE, end_data = 0, got_records, ii, bad_block;


	for(i = 0; !ret_zero && !end_data && fgets(buffer, 1024, fp); i += BLOCKS_X) {
		(*lineno)++;
		crop_by_whitespace(buffer);
		if(!*buffer || *buffer == '#') {
			continue;
		}

		if(!strcmp(buffer, "END_DATA")) {
			end_data = 1;
		} else if(i >= BLOCKS_TOTAL) {
			gui_warning(_("Too many blocks in line %d of %s"), *lineno, filename);
			ret_zero = TRUE;
		} else {
			/* FIXME: Hardcoded to BLOCKS_X == 10 */
			got_records = sscanf(buffer, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &r[0], &r[1], &r[2], &r[3], &r[4], &r[5], &r[6], &r[7], &r[8], &r[9]);

			if(!got_records) {
				gui_warning(_("Syntax error reading level data in line %d of %s"), *lineno, filename);
				ret_zero = TRUE;
			} else if(got_records != BLOCKS_X) {
				gui_warning(_("Expected %d values, got %s at line %d of %s"), got_records, BLOCKS_X, *lineno, filename);
				ret_zero = TRUE;
			} else if((bad_block = verify_raw_data(r, 10, MAX_BLOCK_CODE))) {
				gui_warning(_("Block %d (%d) of line %d of %s is higher than %d"), bad_block, (int) r[bad_block - 1], *lineno, filename, MAX_BLOCK_CODE);
				ret_zero = TRUE;
			} else {
				for(ii = 0; ii < BLOCKS_X; ii++) {
					ret[i + ii] = (gchar) r[ii];
				}
			}
		}
	}

	if(!ret_zero && !end_data && (feof(fp) || ferror(fp))) {
		if(feof(fp)) {
			gui_warning(_("Unexpected EOF while reading level data in file %s"), filename);
		} else {
			gui_warning(_("Error while reading %s: %s"), filename, strerror(errno));
		}

		ret_zero = TRUE;
	}

	if(!ret_zero) {
		g_assert(end_data);
	}

	if(i < BLOCKS_TOTAL) {
		gui_warning(_("Not enough blocks in level data at line %d of %s"), *lineno, filename);
		ret_zero = TRUE;
	}

	if(ret_zero) {
		return 0;
	} else {
		return ret;
	}
}

/* Takes an input line and seperates the supplied tag from the value following
 * it. Does checking for valid characters. On success, returns the string,
 * else NULL */
static gchar *sep_string(gchar *line, gchar *tag, gchar *filename, gint lineno) {
	gchar *buffer, *value;

	/* So we can mess with the line without damaging the original contents
	 */
	buffer = g_strdup(line);

	value = sep_by_tag(buffer, tag, filename, lineno);
	if(value) {
		if(check_valid_chars(value, VALID_STRING, filename, lineno)) {
			value = g_strdup(value);
		} else {
			value = FALSE;
		}
	} else {
		value = FALSE;
	}

	g_free(buffer);

	return value;

}

/* Seperates a value from the supplied tag, showing an error if no value is
 * given. Returns the value on success, NULL on failure. Also crops 
 * surrounding whitespace from the value*/
static gchar *sep_by_tag(gchar *line, gchar *tag, gchar *filename, gint lineno) {
	gint tag_len;
	gchar *ret;

	tag_len = strlen(tag);

	ret = line + tag_len;

	crop_by_whitespace(ret);

	if(!*ret) {
		gui_warning(_("Line %d of %s contains the tag '%s' without a value"), lineno, filename, tag);
		return FALSE;
	} else {
		return ret;
	}
}

/* Checks the validity of a string against a set of allowed characters,
 * showing an error if the check fails. Returns non-NULL if the test passes. */
static gboolean check_valid_chars(gchar *string, gchar *valid, gchar *filename, gint lineno) {
	if(strspn(string, valid) != strlen(string)) {
		gui_warning(_("Line %d of %s contains the value '%s', which contains illegal characters. Legal characters are: '%s'"), lineno, filename, string, valid);
		return FALSE;
	} else {
		return TRUE;
	}
}

/* Takes an input line and seperates the supplied tag from the value following
 * it. Does checking for valid characters. Unlike sep_string, the value in
 * this case must be an unsigned integer. Returns the integer on success, -1
 * otherwise */
static gint sep_uint(gchar *line, gchar *tag, gchar *filename, gint lineno) {
	gchar *buffer, *value;
	gint ret;

	/* So we can mess with the line without damaging the original contents
	 */
	buffer = g_strdup(line);

	value = sep_by_tag(buffer, tag, filename, lineno);
	if(value) {
		if(check_valid_chars(value, VALID_INT, filename, lineno)) {
			ret = atoi(value);
		} else {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	g_free(buffer);

	return ret;
}

/* Crops the leading and ending whitespace of a string. Modifies the original
 * string rather than allocating a new one.
 *
 * This isn't obfuscated, just efficient ;) */
static void crop_by_whitespace(gchar *string) {
	gchar *a, *b;

	for(b = string; *b; b++);
	for(; b > string && strchr(VALID_WHITESPACE, *(b - 1)); b--);
	*b = '\0';	

	/* Move the beginning */
	for(b = string; *b && strchr(VALID_WHITESPACE, *b); b++);
	for(a = string; *b; *a++ = *b++);
	*a = '\0';
}

/* Attempts to find a levelfile matching filename and/or title in the 
 * internal levelfile list. Returns the levelfile on success, NULL otherwise */
static LevelFile *find_levelfile(gchar *filename, gchar *title) {
	GList *curr;
	LevelFile *currlf = NULL;
	LevelFile *ret = NULL;

	for(curr = levelfiles; curr && !ret; curr = g_list_next(curr)) {
		currlf = (LevelFile *) curr->data;
		if((!filename || !strcmp(currlf->filename, filename)) && (!title || !strcmp(currlf->title, title))) {
			ret = currlf;
		}
	}

	return ret;
}

/* Checks that numbers starting from data to n are below or equal to max, 
 * returning n+1 if the test fails, zero otherwise */
static gint verify_raw_data(gint *data, size_t n, gint max) {
	int i;

	for(i = 0; i < n; i++) {
		if(data[i] > max) 
			return i + 1;
	}

	return 0;
}

/* Returns a rawlevel */
RawLevel *leveldata_get(gint level_num) {
	GList *curr;
	gint i;
	g_assert(level_num < num_levels);

	for(i = 0, curr = levels; i < level_num && curr; i++, curr = g_list_next(curr));
	g_assert(curr);

	return (RawLevel *) curr->data;
}

/* Builds a GList of the titles that we have, and returns it. The list is
 * freeable, but the strings are not */
GList *leveldata_titlelist(void) {
	GList *ret = NULL;
	GList *curr;
	
	for(curr = levelfiles; curr; curr = g_list_next(curr)) {
		ret = g_list_prepend(ret, ((LevelFile *) curr->data)->title);
	}

	return ret;
}
