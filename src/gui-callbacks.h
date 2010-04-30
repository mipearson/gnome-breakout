/*
 * Callbacks for events triggered by gui.c. Located here in the interests of
 * cleanliness.
 *
 * Every funciton here should take a GuiInfo * as its data element, unless
 * otherwise specified. Even if the function does not use it currently, it
 * should be passed anyway, incase it is used in the future
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */


void cb_exit_game(GtkWidget *widget, gpointer data);
void cb_show_about_box(GtkWidget *widget, gpointer data);
gint cb_sig_exit_game(GtkWidget *widget, GdkEvent *event, gpointer data);
gint cb_keydown(GtkWidget *widget, GdkEventKey *event, gpointer data);
gint cb_keyup(GtkWidget *widget, GdkEventKey *event, gpointer data);
void cb_new_game(GtkWidget *widget, gpointer data);
void cb_pause_game(GtkWidget *widget, gpointer data);
void cb_end_game(GtkWidget *widget, gpointer data);
void cb_kill_ball(GtkWidget *widget, gpointer data);
void cb_scores(GtkWidget *widget, gpointer data);
void cb_preferences(GtkWidget *widget, gpointer data);
void cb_help(GtkWidget *widget, gpointer data);
gboolean cb_main_focus_change(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean cb_canvas_button_press(GtkWidget *widget, GdkEventButton *event,
		gpointer data);
gboolean cb_grab_focus(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean cb_ungrab_focus(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean cb_hide_pointer(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean cb_show_pointer(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean cb_canvas_pointer(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean cb_null(GtkWidget *widget, void **data);
