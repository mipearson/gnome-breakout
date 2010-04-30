/*
 * Handles loading and storing the flags, and also making decisions
 * based on the flags
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

Flags *load_flags(void);
void save_flags(Flags *flags);
void compute_flags(Flags *flags);
Flags *copy_flags(Flags *flags);
void destroy_flags(Flags *flags);
void remove_flags_levelfile(Flags *flags, gchar *filename);
void add_flags_levelfile(Flags *flags, gchar *filename);

#define MIN_BATSPEED 5
#define MAX_BATSPEED 25

#define MIN_BOUNCE_ENTROPY 0
#define MAX_BOUNCE_ENTROPY 40
