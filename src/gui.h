/*
 * The GNOME-driven breakout GUI
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

void gui_init(Game *game, int argc, char **argv);
void add_to_canvas(Entity *entity);
void remove_from_canvas(Entity *entity);
void gui_update_game(Game *game);
void process_gnome_events(void);
void update_canvas_position(Entity *entity);
void update_canvas_animation(Entity *entity);
void gui_begin_game(void);
void gui_end_game(EndGameStatus status);
gint get_mouse_x_position(void);
void gui_warning(gchar *format, ...);
void gui_error(gchar *format, ...);

/* This holds pointers to objects in the currently running GNOME app. It should
 * only be used in gui.c and gui-callbacks.c */
typedef struct {
	GnomeApp *app;
	GnomeCanvas *canvas;
	GnomeCanvasItem *title_image;
	GnomeCanvasItem *background;
	GtkWidget *vbox;
	GtkWidget *label_hbox1;
	GtkWidget *label_hbox2;
	GtkWidget *score_label;
	GtkWidget *lives_label;
	GtkWidget *level_no_label;
	GtkWidget *level_name_label;
	GtkWidget *level_author_label;
	GtkWidget *level_levelfile_label;
	GtkWidget *appbar;
	GtkWidget *menu_pause;
	GtkWidget *menu_new_game;
	GtkWidget *menu_end_game;
	Game *game;
} GuiInfo;
