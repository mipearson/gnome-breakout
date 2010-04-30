/*
 * The GNOME-driven breakout GUI. Yes, it's messy, but it's also my first
 * ever real GNOME app.
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include "breakout.h"
#include "gui.h"
#include "gui-callbacks.h"
#include "game.h"
#include "anim.h"

/* See gui.h for more info */
static GuiInfo *gui = NULL;

/* Internal functions */
static void init_canvas(void);
static void init_labels(void);
static void init_menus(void);
static void init_statusbar(void);

/* Initialise the interface. */
void gui_init(Game *game, int argc, char **argv) {

	/* Initialise the basic app */
	gui = g_malloc(sizeof(GuiInfo));
	gui->game = game;
	gui->app = (GnomeApp *) gnome_app_new(PACKAGE,
			_("GNOME Breakout"));
	gtk_window_set_policy(GTK_WINDOW (gui->app), FALSE, FALSE, TRUE);
	g_signal_connect(GTK_OBJECT (gui->app), "delete_event",
		GTK_SIGNAL_FUNC (cb_sig_exit_game), gui);

	/* Bind the keypresses */
	g_signal_connect(GTK_OBJECT (gui->app), "key_press_event",
		GTK_SIGNAL_FUNC (cb_keydown), gui);
	g_signal_connect(GTK_OBJECT (gui->app), "key_release_event",
		GTK_SIGNAL_FUNC (cb_keyup), gui);

	/* Pause on focus loss for keyboard control */
	g_signal_connect(GTK_OBJECT(gui->app), "focus_in_event",
			GTK_SIGNAL_FUNC(cb_main_focus_change), gui);
	g_signal_connect(GTK_OBJECT(gui->app), "focus_out_event",
			GTK_SIGNAL_FUNC(cb_main_focus_change), gui);

	/* Main vbox */
	gui->vbox = gtk_vbox_new(FALSE, 0);

	/* Initialise the top statusbar */
	init_labels();

	/* Initialise the canvas */
	gnome_app_set_contents(GNOME_APP (gui->app), gui->vbox);
	init_canvas();

	/* Initialise the statusbar */
	gui->appbar = gnome_appbar_new(FALSE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar(gui->app, gui->appbar);
	init_statusbar();

	/* Initialise the menus */
	init_menus();

	gtk_widget_show_all(GTK_WIDGET (gui->app));
}
	
static void init_canvas(void) {
	GdkPixbuf *image;
        GError *error = NULL;

	/* Push the imlib colormap and visual, and make the canvas */
        /*
	gtk_widget_push_visual(gdk_imlib_get_visual());
	gtk_widget_push_colormap(gdk_imlib_get_colormap());
        */
	gui->canvas = (GnomeCanvas *) gnome_canvas_new();

	/* Set the canvas attributes */
	gnome_canvas_set_pixels_per_unit(gui->canvas, 1);
	gtk_widget_set_usize(GTK_WIDGET(gui->canvas), GAME_WIDTH, GAME_HEIGHT);
	gnome_canvas_set_scroll_region(gui->canvas, 0, 0, GAME_WIDTH,
			GAME_HEIGHT);

	/* Make the canvas background black */
	gui->background = gnome_canvas_item_new(gnome_canvas_root(gui->canvas),
			GNOME_TYPE_CANVAS_RECT, "x1", 0.0, "x2",
			(double) GAME_WIDTH, "y1", 0.0, "y2",
			(double) GAME_HEIGHT, "fill_color", "black", NULL);
	gnome_canvas_item_hide(gui->background);

	/* Add the title image */
	image = gdk_pixbuf_new_from_file(PIXMAPDIR "/title.png", &error);
	if(!image)
		gb_error("Cannot find title image " PIXMAPDIR "/title.png: %s",
                            error->message);

        //gdk_imlib_render(image, image->rgb_width, image->rgb_height);
	gui->title_image = gnome_canvas_item_new(
			gnome_canvas_root(gui->canvas),
			GNOME_TYPE_CANVAS_PIXBUF, "pixbuf", image, 
			"x", 0.0, "y", 0.0,
			"width", (double) GAME_WIDTH, 
			"height", (double) GAME_HEIGHT,
			"anchor", GTK_ANCHOR_NORTH_WEST,
			NULL);

	gnome_canvas_update_now(gui->canvas);

	/* Hide pointer and automatic pause */
	g_signal_connect(GTK_OBJECT (gui->canvas), "enter-notify-event",
			GTK_SIGNAL_FUNC (cb_canvas_pointer), gui);
	g_signal_connect(GTK_OBJECT (gui->canvas), "leave-notify-event",
			GTK_SIGNAL_FUNC (cb_canvas_pointer), gui);

	/* Bind the mouse fire events */
	g_signal_connect(GTK_OBJECT(gui->canvas), "button_press_event",
			GTK_SIGNAL_FUNC(cb_canvas_button_press), gui);

	gtk_box_pack_start(GTK_BOX(gui->vbox), GTK_WIDGET(gui->canvas),
			FALSE, FALSE, 0);
}

static void init_statusbar(void) {
	GtkWidget *vseparator1;

	gui->score_label = gtk_label_new(_("Score: 0"));
	gtk_label_set_justify(GTK_LABEL(gui->score_label), GTK_JUSTIFY_LEFT);
	gui->lives_label = gtk_label_new(_("Lives: 0"));
	gtk_label_set_justify(GTK_LABEL(gui->lives_label), GTK_JUSTIFY_LEFT);

	vseparator1 = gtk_vseparator_new();

	gtk_widget_set_sensitive(gui->score_label, FALSE);
	gtk_widget_set_sensitive(gui->lives_label, FALSE);

	gtk_box_pack_end(GTK_BOX(gui->appbar), gui->score_label,
			FALSE, FALSE, 5);
	gtk_box_pack_end(GTK_BOX(gui->appbar), vseparator1, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(gui->appbar), gui->lives_label,
			FALSE, FALSE, 5);
}

/* Adds an entity to the gnome canvas */
void add_to_canvas(Entity *entity) {
	entity->animation.canvas_item = gnome_canvas_item_new(
			gnome_canvas_root(GNOME_CANVAS(gui->canvas)),
			GNOME_TYPE_CANVAS_PIXBUF,
			"pixbuf", entity->animation.pixmaps[entity->animation.frame_no],
			"x", (double) entity->geometry.x1,
			"y", (double) entity->geometry.y1,
			"width", (double) entity->geometry.x2 - entity->geometry.x1,
			"height", (double) entity->geometry.y2 - entity->geometry.y1,
			"anchor", GTK_ANCHOR_NORTH_WEST,
			NULL);
}

/* Remove an entity from the gnome canvas. Does not assume that the entity
 * actually has a canvas_item */
void remove_from_canvas(Entity *entity) {
	if(entity->animation.canvas_item) {
		gtk_object_destroy(GTK_OBJECT(entity->animation.canvas_item));
		entity->animation.canvas_item = NULL;
	}
}

/* Process all pending gnome events */
void process_gnome_events(void) {
	while(gtk_events_pending())
		gtk_main_iteration();
}

/* Updates normal game-related GUI elements. Should be run at least once
 * every iteration */
void gui_update_game(Game *game) {
	char *score, *lives, *level_no, *level_name, *level_levelfile, *level_author;
	static gint32 oldscore = -1;
	static gint oldlives = -1, oldlevel = -1;

	if(oldlevel != game->level_no) {
		oldlevel = game->level_no;

		level_no = g_strdup_printf(_("No: %d"), game->level_no + 1);
		gtk_label_set_text(GTK_LABEL(gui->level_no_label), level_no);
		g_free(level_no);

		if(game->level) {
			level_name = g_strdup_printf(_("Name: %s"), game->level->name);
			level_author = g_strdup_printf(_("Author: %s"), game->level->author);
			level_levelfile = g_strdup_printf(_("Levelfile: %s"), game->level->levelfile_title);
		} else {
			level_name = g_strdup(_("Name:"));
			level_author = g_strdup(_("Author:"));
			level_levelfile = g_strdup(_("Filename:"));
		}

		gtk_label_set_text(GTK_LABEL(gui->level_name_label), level_name);
		gtk_label_set_text(GTK_LABEL(gui->level_author_label), level_author);
		gtk_label_set_text(GTK_LABEL(gui->level_levelfile_label), level_levelfile);

		g_free(level_name);
		g_free(level_author);
		g_free(level_levelfile);
	}

	if(oldscore != game->score) {
		oldscore = game->score;
		score = g_strdup_printf(_("Score: %d"), game->score);
		gtk_label_set_text(GTK_LABEL(gui->score_label), score);
		g_free(score);
	}

	if(oldlives != game->lives) {
		oldlives = game->lives;
		lives = g_strdup_printf(_("Lives: %d"), game->lives);
		gtk_label_set_text(GTK_LABEL(gui->lives_label), lives);
		g_free(lives);
	}

	gnome_canvas_update_now(gui->canvas);
	return;
}

/* Assumes that we already have an appbar */
static void init_menus() {
	GnomeUIInfo game_menu[] = {
		GNOMEUIINFO_MENU_NEW_GAME_ITEM(cb_new_game, gui),
		GNOMEUIINFO_MENU_PAUSE_GAME_ITEM(cb_pause_game, gui),
		GNOMEUIINFO_MENU_END_GAME_ITEM(cb_end_game, gui),
		GNOMEUIINFO_SEPARATOR,
		GNOMEUIINFO_MENU_PREFERENCES_ITEM(cb_preferences, gui),
		/* FIXME: Remove this when appropriate */
		GNOMEUIINFO_ITEM_DATA(_("_Kill ball"), 
				_("Kill the current ball if it gets stuck"),
				cb_kill_ball, gui, NULL),
		GNOMEUIINFO_MENU_SCORES_ITEM(cb_scores, gui),
		GNOMEUIINFO_SEPARATOR,
		GNOMEUIINFO_MENU_EXIT_ITEM(cb_exit_game, gui),
		GNOMEUIINFO_END
	};

	GnomeUIInfo help_menu[] = {
		GNOMEUIINFO_ITEM_DATA("_Help", "Help on this application",
				cb_help, gui, NULL),
		GNOMEUIINFO_MENU_ABOUT_ITEM(cb_show_about_box, gui),
		GNOMEUIINFO_END
	};

	GnomeUIInfo menubar[] = {
		GNOMEUIINFO_MENU_GAME_TREE(game_menu),
		GNOMEUIINFO_MENU_HELP_TREE(help_menu),
		GNOMEUIINFO_END
	};

	gnome_app_create_menus(gui->app, menubar);
	gnome_app_install_menu_hints(gui->app, menubar);
	gui->menu_new_game = game_menu[0].widget;
	gui->menu_pause = game_menu[1].widget;
	gui->menu_end_game = game_menu[2].widget;
	gtk_widget_set_sensitive(gui->menu_pause, FALSE);
	gtk_widget_set_sensitive(gui->menu_end_game, FALSE);
}

/* Updates the position of an item on the canvas. Assumes that the width or
 * height of the object hasn't changed */
void update_canvas_position(Entity *entity) {
	if(entity->animation.canvas_item) {
		gnome_canvas_item_set(GNOME_CANVAS_ITEM(entity->animation.canvas_item),
			"x", (double) entity->geometry.x1,
			"y", (double) entity->geometry.y1,
			NULL);
	}
}

/* Updates the current pixmap of an item on the canvas */
void update_canvas_animation(Entity *entity) {
	g_assert(entity->animation.pixmaps[entity->animation.frame_no]);
	if(entity->animation.canvas_item) {
		gnome_canvas_item_set(GNOME_CANVAS_ITEM(entity->animation.canvas_item),
				"pixbuf", entity->animation.pixmaps[entity->animation.frame_no],
				NULL);
	}
}

/* Tell the gui that the game has ended, and that we should display the title
 * This must be called by everything that calls game.c:end_game, and it must
 * be called before calling game.c:end_game */
void gui_end_game(EndGameStatus status) {
	int pos;
	char *title = NULL;

	gnome_canvas_item_hide(gui->background);
        gnome_canvas_item_show(gui->title_image);
        gnome_canvas_update_now(gui->canvas);

        gtk_widget_set_sensitive(gui->score_label, FALSE);
        gtk_widget_set_sensitive(gui->lives_label, FALSE);
        gtk_widget_set_sensitive(gui->level_no_label, FALSE);
        gtk_widget_set_sensitive(gui->level_name_label, FALSE);
        gtk_widget_set_sensitive(gui->level_author_label, FALSE);
        gtk_widget_set_sensitive(gui->level_levelfile_label, FALSE);
	gtk_widget_set_sensitive(gui->menu_pause, FALSE);
	gtk_widget_set_sensitive(gui->menu_end_game, FALSE);

	pos = gnome_score_log((gfloat) gui->game->score, NULL, TRUE);
	switch(status) {
		case ENDGAME_WIN :
			title = _("GNOME Breakout: You win!");
			break;
		case ENDGAME_LOSE :
			title = _("GNOME Breakout: You lose!");
			break;
		case ENDGAME_MENU :
			break;
		default :
			g_assert_not_reached();
	}

	if(title)
		gnome_scores_display(title, PACKAGE, NULL, pos);
}

/* Tell the gui that the game has begun, and that we should hide the title */
void gui_begin_game(void) {
        gnome_canvas_item_hide(gui->title_image);
        gnome_canvas_item_show(gui->background);
        gnome_canvas_update_now(gui->canvas);

        gtk_widget_set_sensitive(gui->score_label, TRUE);
        gtk_widget_set_sensitive(gui->lives_label, TRUE);
        gtk_widget_set_sensitive(gui->level_no_label, TRUE);
        gtk_widget_set_sensitive(gui->level_name_label, TRUE);
        gtk_widget_set_sensitive(gui->level_author_label, TRUE);
        gtk_widget_set_sensitive(gui->level_levelfile_label, TRUE);
	gtk_widget_set_sensitive(gui->menu_pause, TRUE);
	gtk_widget_set_sensitive(gui->menu_end_game, TRUE);
}

gint get_mouse_x_position(void) {
	gint x;
	gint y;
	gtk_widget_get_pointer(GTK_WIDGET(gui->app), &x, &y);
	return x;
}

/* Displays a warning dialog box and prints the warning to STDERR */
void gui_warning(gchar *format, ...) {
	GtkWidget *mbox;
	gchar buffer[1024];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buffer, 1024, format, ap);
	va_end(ap);

	if(gui && gui->app) {
		mbox = (GtkWidget *) gtk_message_dialog_new(
				GTK_WINDOW(gui->app),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_CLOSE,
				buffer);
		gtk_dialog_run(GTK_DIALOG(mbox));
		gtk_widget_destroy(mbox);
	} else {
		g_warning("Attempted to display previous warning message before GUI initialisation");
	}
}

/* Displays an error dialog box */
void gui_error(gchar *format, ...) {
	GtkWidget *mbox;
	gchar buffer[1024];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buffer, 1024, format, ap);
	va_end(ap);

	if(gui && gui->app) {
		mbox = (GtkWidget *) gtk_message_dialog_new(
				GTK_WINDOW(gui->app),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				buffer);
		gtk_dialog_run(GTK_DIALOG(mbox));
		gtk_widget_destroy(mbox);
	} else {
		g_warning("Attempted to display error message \"%s\" before gui activation", buffer);
	}
}

/* Sets up the level labels at the top of the screen */
static void init_labels(void) {
	GtkWidget *vsep1, *vsep2, *hsep1;

	gui->label_hbox1 = gtk_hbox_new(FALSE, 0);
	gui->label_hbox2 = gtk_hbox_new(FALSE, 0);
	gui->level_no_label = gtk_label_new(_("No:"));
	gui->level_name_label = gtk_label_new(_("Name:"));
	gui->level_author_label = gtk_label_new(_("Author:"));
	gui->level_levelfile_label = gtk_label_new(_("Levelfile:"));
	vsep1 = gtk_vseparator_new();
	vsep2 = gtk_vseparator_new();
	hsep1 = gtk_hseparator_new();

	gtk_label_set_justify(GTK_LABEL(gui->level_no_label), GTK_JUSTIFY_LEFT);
	gtk_label_set_justify(GTK_LABEL(gui->level_name_label), GTK_JUSTIFY_LEFT);
	gtk_label_set_justify(GTK_LABEL(gui->level_author_label), GTK_JUSTIFY_LEFT);
	gtk_label_set_justify(GTK_LABEL(gui->level_levelfile_label), GTK_JUSTIFY_LEFT);

	gtk_widget_set_sensitive(gui->level_no_label, FALSE);
	gtk_widget_set_sensitive(gui->level_name_label, FALSE);
	gtk_widget_set_sensitive(gui->level_levelfile_label, FALSE);
	gtk_widget_set_sensitive(gui->level_author_label, FALSE);

	gtk_box_pack_start(GTK_BOX(gui->label_hbox1), gui->level_no_label, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(gui->label_hbox1), vsep1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gui->label_hbox1), gui->level_name_label, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(gui->label_hbox2), gui->level_author_label, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(gui->label_hbox2), vsep2, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gui->label_hbox2), gui->level_levelfile_label, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(gui->vbox), gui->label_hbox1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gui->vbox), hsep1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gui->vbox), gui->label_hbox2, TRUE, TRUE, 0);

}
