/*
 * Callbacks for events triggered by gui.c. Located here in the interests of
 * cleanliness.
 *
 * Every funciton here should take a GuiInfo * as its data element, unless
 * otherwise specified. Even if the function does not use it currently, it
 * should be passed anyway, incase it is used in the future.
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include "breakout.h"
#include "gui.h"
#include "gui-callbacks.h"
#include "gui-preferences.h"
#include "game.h"
#include "ball.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>

//#define NEXTLEVEL_KEY 1

/* We need this here because gnome-breakout tends to output alot of nasty
 * warning messages if we quit without killing off alot of canvas objects */
void cb_exit_game(GtkWidget *widget, gpointer data) {
	GuiInfo *gui;

        gui = (GuiInfo *) data;
	if(gui->game->state != STATE_STOPPED) {
		end_game(gui->game, ENDGAME_MENU);
	}

        gtk_object_destroy(GTK_OBJECT(gui->background));
        gtk_object_destroy(GTK_OBJECT(gui->title_image));

        gtk_main_quit();
}

/* And we need this because the event mechanism is different from the menu
 * mechanism */
gint cb_sig_exit_game(GtkWidget *widget, GdkEvent *event, gpointer data) {
	cb_exit_game(NULL, data);

	return FALSE;
}

/* Show the about box. I wonder, does the about box dialog free itself? */
void cb_show_about_box(GtkWidget *widget, gpointer data) {
        static GtkWidget *dlg;
	GuiInfo *gui;

        const gchar *authors[] = {
                "Development:",
		"Michael Pearson <mipearson@internode.on.net>",
		"",
		"Additional Levels:",
		"Marisa Mack <marisa@teleport.com>",
		"Mathieu Dutour <dutour@clipper.ens.fr>",
		"",
		"Additional Graphics:",
		"N0mada <n0mada@wanadoo.es>",
                NULL };
	const gchar comment[] = N_(
		"The classic arcade game Breakout."
                );

        const gchar *translator_credits = _("translator credits");

        gchar *use_tc = NULL;
        if(strcmp(translator_credits, "translator credits")) {
            /* This is a translation, show appropriate credits */
            use_tc = (gchar *) translator_credits;
        }

	if(dlg) {
		gdk_window_show(dlg->window);
		gdk_window_raise(dlg->window);
		gtk_widget_grab_focus(GTK_WIDGET(dlg));
	} else {
        	dlg = gnome_about_new(
                        _("GNOME Breakout"), // name
                        VERSION,             // version
                	_("Copyright (c) 2000-2003 Michael Pearson"), // cp
			_(comment),         // comments
                       	authors,            // authors
                        NULL,               // documenters
                        use_tc,             // translator credits
                        NULL);              // logo

		gui = (GuiInfo *) data;
		g_signal_connect(GTK_OBJECT(dlg), "destroy",
				GTK_SIGNAL_FUNC(cb_null), &dlg);
        	gtk_widget_show(dlg);
	}
}

/* Called when the user holds down a key. If it's a key we recognise, pass it
 * on to the relevant game.c function */
gint cb_keydown(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	GuiInfo *gui;
	Game *game;
	gui = (GuiInfo *) data;
	game = gui->game;

	if(game->flags->keyboard_control) {
        	if(event->keyval == game->flags->left_key) {
                	key_left_pressed(game);
        	} else if(event->keyval == game->flags->right_key) {
                	key_right_pressed(game);
        	} else if(event->keyval == game->flags->fire1_key) {
			key_fire1_pressed(game);
		} else if(event->keyval == game->flags->fire2_key) {
			key_fire2_pressed(game);
		}
	}

        return FALSE;
}

/* Same as above, but for when a key is released */
gint cb_keyup(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	Game *game;
	GuiInfo *gui;
	gui = (GuiInfo *) data;
	game = gui->game;

	if(game->flags->keyboard_control) {
        	if(event->keyval == game->flags->left_key) {
      	        	key_left_released(game);
	        } else if(event->keyval == game->flags->right_key) {
	                key_right_released(game);
       		} else if(event->keyval == game->flags->fire1_key) {
			key_fire1_released(game);
		} else if(event->keyval == game->flags->fire2_key) {
			key_fire2_released(game);
		}
	}
#ifdef NEXTLEVEL_KEY
#warning NEXTLEVEL_KEY cheat is enabled.
	if(event->keyval == GDK_L && game->state == STATE_RUNNING)
		game->powerup_next_level = TRUE;
#endif

        return FALSE;
}

/* Hides the title image, and starts a new game */
void cb_new_game(GtkWidget *widget, gpointer data) {
	GuiInfo *gui;
	gui = (GuiInfo *) data;

	/* Hide the title */
        /*gnome_canvas_item_hide(gui->title_image);
        gnome_canvas_item_show(gui->background);
        gnome_canvas_update_now(gui->canvas);*/

	if(gui->game->state != STATE_STOPPED) {
		end_game(gui->game, ENDGAME_MENU);
	}

	run_game(gui->game);
}

/* Ends the game and shows the title */
void cb_end_game(GtkWidget *widget, gpointer data) {
	GuiInfo *gui;
	gui = (GuiInfo *) data;

	if(gui->game->state != STATE_STOPPED) {
		end_game(gui->game, ENDGAME_MENU);
	}
}

/* Just calls pause_game. Included for consistency */
void cb_pause_game(GtkWidget *widget, gpointer data) {
	GuiInfo *gui;
	gui = (GuiInfo *) data;

	if(gui->game->pause_state & PAUSE_MENU) {
		pause_game(gui->game, PAUSE_MENU, 1);
	} else {
		pause_game(gui->game, PAUSE_MENU, 0);
	}
}

/* Calls make_preferences_box. Again, included for consistency */
void cb_preferences(GtkWidget *widget, gpointer data) {
	GuiInfo *gui;
	gui = (GuiInfo *) data;

	make_preferences_box(gui);
}

/* For now, just prints a pretty message */
void cb_help(GtkWidget *widget, gpointer data) {
	GuiInfo *gui;
	GtkWidget *mbox;

	gui = (GuiInfo *) data;
        // FIXME!!
	mbox = (GtkWidget *) gnome_ok_dialog_parented(_("Help hasn't been implimented yet. So go ahead and write some for me :)"), GTK_WINDOW(gui->app));
	gtk_widget_show(mbox);
}

/* The application has gained or lost focus */
gboolean cb_main_focus_change(GtkWidget *widget, GdkEvent *event, gpointer data) {
	GuiInfo *gui;
	GdkEventFocus *fevent;
	
	gui =  (GuiInfo *) data;
	fevent = (GdkEventFocus *) event;

	if(fevent->in) {
		/* Gained focus */
		pause_game(gui->game, PAUSE_FOCUS, 1);
	} else {
		/* Lost Focus */
		if(gui->game->flags->pause_on_focus) {
			pause_game(gui->game, PAUSE_FOCUS, 0);
		}
	}

	return FALSE;
}

gboolean cb_canvas_button_press(GtkWidget *widget, GdkEventButton *event,
		gpointer data) {
	GuiInfo *gui;
	gui = (GuiInfo *) data;

	if(gui->game->flags->mouse_control) {
		if(event->button == 1)
			key_fire1_pressed(gui->game);
		else if(event->button == 3)
			key_fire2_pressed(gui->game);
	}

	return FALSE;
}

/* Kills the current ball, if it gets stuck inside a block */
void cb_kill_ball(GtkWidget *widget, gpointer data) {
	GuiInfo *gui;
	gui = (GuiInfo *) data;

	if(gui->game->balls)
		ball_die(gui->game, (Ball *) gui->game->balls->data);
}

/* Displays the scores */
void cb_scores(GtkWidget *widget, gpointer data) {
	gnome_scores_display(_("GNOME Breakout: Scores"), PACKAGE, NULL, 0);
}

/* The pointer has either entered or left the canvas */
gboolean cb_canvas_pointer(GtkWidget *widget, GdkEvent *event, gpointer data) {
	GuiInfo *gui;
	gui = (GuiInfo *) data;

	if(event->type == GDK_ENTER_NOTIFY && gui->game->state != STATE_STOPPED) {
		/*if(!gui->game->flags->keyboard_control && !gui->game->flags->pause_on_focus_loss) {
			cb_grab_focus(GTK_WIDGET(gui->canvas), NULL, NULL);
		}*/
		if(gui->game->flags->hide_pointer) {
			cb_hide_pointer(GTK_WIDGET(gui->canvas), NULL, NULL);
		}
		pause_game(gui->game, PAUSE_POINTER, 1);
	} else if(event->type == GDK_LEAVE_NOTIFY) {
		/*if(!gui->game->flags->keyboard_control && !gui->game->flags->pause_on_focus_loss) {
			cb_ungrab_focus(GTK_WIDGET(gui->canvas), NULL, NULL);
		}*/
		cb_show_pointer(GTK_WIDGET(gui->canvas), NULL, NULL);
		if(gui->game->flags->pause_on_pointer) {
			pause_game(gui->game, PAUSE_POINTER, 0);
		}
	
	}
	
	return TRUE;
}

/* Maybe there are GTK+ functions for the following stuff, but I don't know them */
gboolean cb_grab_focus(GtkWidget *widget, GdkEvent *event, gpointer data) {
	XGrabPointer(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window),
		GDK_WINDOW_XWINDOW(GTK_WIDGET(widget)->window), 1,
		PointerMotionMask, GrabModeAsync, GrabModeAsync,
		GDK_WINDOW_XWINDOW(GTK_WIDGET(widget)->window),
		None, CurrentTime);

	return TRUE;
}

gboolean cb_ungrab_focus(GtkWidget *widget, GdkEvent *event, gpointer data) {
	XUngrabPointer(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window), CurrentTime);

	return TRUE;
}

gboolean cb_show_pointer(GtkWidget *widget, GdkEvent *event, gpointer data) {
	XUndefineCursor(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window),
		GDK_WINDOW_XWINDOW(GTK_WIDGET(widget)->window));

	return TRUE;
}

gboolean cb_hide_pointer(GtkWidget *widget, GdkEvent *event, gpointer data) {
	static Cursor cursor = 0;
	XColor colour;
	Pixmap cursorPixmap;

	colour.pixel = WhitePixel(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window),
		DefaultScreen(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window)));
	XQueryColor(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window), 
		DefaultColormap(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window),
		DefaultScreen(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window))), &colour);
	if(cursor) {
		XFreeCursor(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window), cursor);
	}
	cursorPixmap = XCreatePixmap(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window),
		GDK_WINDOW_XWINDOW(GTK_WIDGET(widget)->window), 1, 1, 1);
	cursor = XCreatePixmapCursor(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window),
		cursorPixmap, cursorPixmap, &colour, &colour, 0, 0);
	if(cursorPixmap) {
		XFreePixmap(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window), cursorPixmap);
	}
	XDefineCursor(GDK_WINDOW_XDISPLAY(GTK_WIDGET(widget)->window),
		GDK_WINDOW_XWINDOW(GTK_WIDGET(widget)->window), cursor);

	return TRUE;
}

/* Nulls the variable passed to it */
gboolean cb_null(GtkWidget *w, void **data) {
	*data = NULL;

	return TRUE;
}
