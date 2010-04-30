/*
 * Main game iteration, and misc functions
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

void iterate_game(Game *game);
void lose_life(Game *game);
void run_game(Game *game);
void pause_game(Game *game, PauseType type, gboolean unpause);
void end_game(Game *game, EndGameStatus status);
void next_level(Game *game);
void key_left_pressed(Game *game);
void key_left_released(Game *game);
void key_right_pressed(Game *game);
void key_right_released(Game *game);
void key_fire1_pressed(Game *game);
void key_fire1_released(Game *game);
void key_fire2_pressed(Game *game);
void key_fire2_released(Game *game);
void mouse_moved(Game *game, int position);
void new_life(Game *game);
int process_events(Game *game);
void destroy_pending_events_list(GList *pending_events);
int check_newlife_score(Game *game);
