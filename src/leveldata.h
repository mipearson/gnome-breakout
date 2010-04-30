/*
 * Functions for loading and parsing custom level files.
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is license under the GNU General Public License. See the file
 * "COPYING" for more details
 */

#define BLOCK_NONE_CODE 0
#define BLOCK_DEFAULT_CODE 1
#define BLOCK_INVINCIBLE_CODE 2
#define BLOCK_STRONG_1_CODE 3
#define BLOCK_STRONG_2_CODE 4 
#define BLOCK_STRONG_3_CODE 5 
#define BLOCK_EXPLODE_CODE 6
#define MAX_BLOCK_CODE 6

gchar *leveldata_add(char *filename);
gchar *leveldata_remove(char *title);
RawLevel *leveldata_get(gint level_num);
GList *leveldata_titlelist(void);
gint leveldata_num_levels(void);
