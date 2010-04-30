/*
 * The functions which create and handle the preferences dialog.
 * This is a damn long file, so I've sectioned it off somewhat with comments.
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include "breakout.h"
#include "gui.h"
#include "gui-preferences.h"
#include "game.h"
#include "flags.h"
#include "leveldata.h"

static GtkWidget *dialog = NULL;
static gboolean flags_changed = FALSE;
static Flags *newflags;

/* Internal data structures */
typedef struct {
	Game *game;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkListStore *level_list_store;
	GtkTreeView *level_list_view;
	GtkWidget *level_list_scrollpane;
	GtkWidget *level_list_hbox;
	GtkWidget *button_hbox;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *fsel;
	gchar *level_list_selection;
	GtkTreeIter level_list_iter;
} LevelFrame;

/* Internal functions */
static void init_preferences_box(GuiInfo *gui, GtkNotebook **window_notebook);
static void init_game_page(GuiInfo *gui, GtkNotebook *window_notebook);
static void init_control_page(GuiInfo *gui, GtkNotebook *window_notebook);
static void populate_level_list(LevelFrame *lf, GList *filenames);
static void apply_preferences(GuiInfo *gui);
static void set_flags_changed(gboolean status);

/* Callbacks */
static void cb_pref_destroy(GtkWidget *widget, gpointer data);
static void cb_diff(GtkWidget *widget, gpointer data);
static void cb_ctrl_key(GtkWidget *widget, gpointer data);
static void cb_ctrl_mouse(GtkWidget *widget, gpointer data);
static void cb_bat_speed(GtkWidget *widget, gpointer data);
static void cb_bounce_entropy(GtkWidget *widget, gpointer data);
static void cb_key_left(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void cb_key_right(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void cb_key_fire1(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void cb_key_fire2(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void cb_pref_response(GtkWidget *widget, gint response_id, gpointer data);
static void cb_level_list_changed(GtkTreeSelection *selection,
		gpointer data);
static void cb_level_add(GtkWidget *widget, gpointer data);
static void cb_level_remove(GtkWidget *widget, gpointer data);
static void cb_filesel_ok(GtkWidget *w, gpointer data);
static void cb_filesel_cancel(GtkWidget *w, gpointer data);
static void cb_checkbox_toggled(GtkWidget *w, gboolean *flag);

void make_preferences_box(GuiInfo *gui) {
	GtkNotebook *window_notebook;
	
	/* Don't let the user run two preferences boxes at the same time */
	if(dialog) {
		gdk_window_show( dialog->window );
		gdk_window_raise( dialog->window);
		gtk_widget_grab_focus(GTK_WIDGET(dialog));
		return;
	}

	if(gui->game->flags->pause_on_pref) {
		pause_game(gui->game, PAUSE_PREF, 0);
	}

	newflags = copy_flags(gui->game->flags);

	init_preferences_box(gui, &window_notebook);
	init_game_page(gui, window_notebook); 
	init_control_page(gui, window_notebook);
	set_flags_changed(FALSE);

	gtk_widget_show_all(dialog);
}

static void cb_pref_response(GtkWidget *widget, gint response_id,
		gpointer data) {
	GuiInfo *gui;
	gui = (GuiInfo *) data;

	if(response_id == GTK_RESPONSE_APPLY && flags_changed) {
		apply_preferences(gui);
	} else if(response_id == GTK_RESPONSE_OK) {
		if(flags_changed) {
			apply_preferences(gui);
		}
		gtk_widget_destroy(widget);
	} else if(response_id == GTK_RESPONSE_CLOSE) {
		gtk_widget_destroy(widget);
	}
}

/* Apply has been clicked */
static void apply_preferences(GuiInfo *gui) {
	//GuiInfo *gui;
	//gui = (GuiInfo *) data;

	save_flags(newflags);
	destroy_flags(gui->game->flags);
	gui->game->flags = newflags;
	newflags = copy_flags(newflags);
	set_flags_changed(FALSE);
}

/* Changed the behaviour of the dialog box depending on whether the flags
 * have been changed since the last commit */
static void set_flags_changed(gboolean status) {
	flags_changed = status;
	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
			GTK_RESPONSE_APPLY, status);
}

/* Preferences dialog has been destroyed */
static void cb_pref_destroy(GtkWidget *widget, gpointer data) {
	GuiInfo *gui;
	gui = (GuiInfo *) data;

	dialog = NULL;

	destroy_flags(newflags);

	pause_game(gui->game, PAUSE_PREF, 1);
}

/*
 * Page generation
 */

/* Initialise the actual dialog box, but don't add anything */
static void init_preferences_box(GuiInfo *gui, GtkNotebook **window_notebook) {
        dialog = gtk_dialog_new_with_buttons(
                _("GNOME Breakout Preferences"), GTK_WINDOW(gui->app),
                0, GTK_STOCK_APPLY,
		GTK_RESPONSE_APPLY, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	g_signal_connect(GTK_OBJECT(dialog), "destroy",
			G_CALLBACK(cb_pref_destroy), gui);
	g_signal_connect(GTK_OBJECT(dialog), "response",
			G_CALLBACK(cb_pref_response), gui);

	*window_notebook = GTK_NOTEBOOK(gtk_notebook_new());
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
        		GTK_WIDGET(*window_notebook));
	
	
}

/* Initialise the "game" section of the preferences box */
void init_game_page(GuiInfo *gui, GtkNotebook *window_notebook) {
	GtkWidget *gametablabel;
	GtkWidget *gamehbox;
	GtkWidget *gamevbox1;
	GtkWidget *gamevbox2;

	GtkWidget *bounce_entropy_label;
	GtkWidget *bounce_entropy_hbox;
	GtkObject *bounce_entropy_adjustment;
	GtkWidget *bounce_entropy_sbutton;
	GtkWidget *hide_pointer_check;

	GtkWidget *pause_frame;
	GtkWidget *pause_vbox;
	GtkWidget *pause_on_pointer_check;
	GtkWidget *pause_on_focus_check;
	GtkWidget *pause_on_pref_check;

	static LevelFrame lf;
	GList *levelfiles;
	
	GtkWidget *difficulty_frame;
	GtkWidget *difficulty_vbox;
	GtkWidget *difficulty_easy_radio;
	GtkWidget *difficulty_medium_radio;
	GtkWidget *difficulty_hard_radio;

	GtkTreeSelection *level_list_selection;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* Reset levelframe */
	memset(&lf, 0, sizeof(lf));

	/* Vbox and Tablabel - Constructor */
	gametablabel = gtk_label_new(_("Game"));
	gamehbox = gtk_hbox_new(FALSE, GNOME_PAD);
	gamevbox1 = gtk_vbox_new(FALSE, GNOME_PAD);
	gamevbox2 = gtk_vbox_new(FALSE, GNOME_PAD);

	/* Vbox and Tablabel - Packing */
	gtk_box_pack_start(GTK_BOX(gamehbox), gamevbox1, TRUE, TRUE, GNOME_PAD);
	gtk_box_pack_start(GTK_BOX(gamehbox), gamevbox2, TRUE, TRUE, GNOME_PAD);

	/* Vbox and Tablabel - Show */
	gtk_widget_show(gametablabel);
	gtk_widget_show(gamehbox);
	gtk_widget_show(gamevbox1);
	gtk_widget_show(gamevbox2);

	/* Level Frame - Constructor */
	lf.game = gui->game;
	lf.frame = gtk_frame_new(_("Levels"));
	lf.vbox = gtk_vbox_new(FALSE, 0);
	lf.level_list_store = gtk_list_store_new(1, G_TYPE_STRING);
	lf.level_list_view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(
			GTK_TREE_MODEL(lf.level_list_store)));
	//lf.warning_label = gtk_label_new(_("Levels cannot be added or removed\nwhile the game is running."));
	lf.button_hbox = gtk_hbox_new(TRUE, 0);
	lf.level_list_hbox = gtk_hbox_new(TRUE, 0);
	lf.add_button = gtk_button_new_with_label(_("Add"));
	lf.remove_button = gtk_button_new_with_label(_("Remove"));
	lf.game = gui->game;
	lf.level_list_scrollpane = gtk_scrolled_window_new(NULL, NULL);
	lf.level_list_selection = NULL;

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
		renderer, "text", 0, NULL);
	gtk_tree_view_append_column(lf.level_list_view, column);

	/* Level Frame - Settings */
	level_list_selection = gtk_tree_view_get_selection(lf.level_list_view);
	gtk_tree_selection_set_mode(level_list_selection, GTK_SELECTION_SINGLE);

	gtk_frame_set_shadow_type(GTK_FRAME(lf.frame), GTK_SHADOW_ETCHED_IN);
	//gtk_clist_set_shadow_type(GTK_CLIST(lf.clist), GTK_SHADOW_ETCHED_IN);
	//gtk_clist_set_column_width(GTK_CLIST(lf.clist), 0, 200);
	levelfiles = leveldata_titlelist();
	populate_level_list(&lf, levelfiles);
	g_list_free(levelfiles);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (lf.level_list_scrollpane), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_tree_view_set_headers_visible(lf.level_list_view, FALSE);
	
	/* Level Frame - Signals */
	g_signal_connect(G_OBJECT(level_list_selection), "changed", G_CALLBACK(cb_level_list_changed), (gpointer) &lf);
	/*
	g_signal_connect_object(GTK_OBJECT(lf.add_button), "clicked", GTK_SIGNAL_FUNC(cb_level_add), (gpointer) &lf, 0);
	g_signal_connect_object(GTK_OBJECT(lf.remove_button), "clicked", GTK_SIGNAL_FUNC(cb_level_remove), (gpointer) &lf, 0);
	*/
	g_signal_connect(GTK_OBJECT(lf.add_button), "clicked", GTK_SIGNAL_FUNC(cb_level_add), (gpointer) &lf);
	g_signal_connect(GTK_OBJECT(lf.remove_button), "clicked", GTK_SIGNAL_FUNC(cb_level_remove), (gpointer) &lf);


	/* Level Frame - Sensitivity */
	if(gui->game->state != STATE_STOPPED) {
		gtk_widget_set_sensitive(GTK_WIDGET(lf.add_button), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(lf.level_list_view), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(lf.level_list_scrollpane), FALSE);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(lf.remove_button), FALSE);

	/* Level Frame - Packing */	
	gtk_box_pack_start(GTK_BOX(gamevbox1), lf.frame, TRUE, TRUE, GNOME_PAD);
	gtk_container_add(GTK_CONTAINER(lf.frame), lf.vbox);
	//gtk_box_pack_start_defaults(GTK_BOX(lf.vbox), lf.warning_label);
	gtk_box_pack_start(GTK_BOX(lf.vbox), lf.level_list_hbox, TRUE, TRUE, GNOME_PAD);
	gtk_box_pack_start(GTK_BOX(lf.vbox), lf.button_hbox, FALSE, FALSE, GNOME_PAD);
	gtk_container_add(GTK_CONTAINER(lf.level_list_scrollpane), GTK_WIDGET(lf.level_list_view));
	gtk_box_pack_start(GTK_BOX(lf.level_list_hbox), lf.level_list_scrollpane, TRUE, TRUE, GNOME_PAD);
	gtk_box_pack_start(GTK_BOX(lf.button_hbox), lf.add_button, TRUE, TRUE, GNOME_PAD);
	gtk_box_pack_start(GTK_BOX(lf.button_hbox), lf.remove_button, TRUE, TRUE, GNOME_PAD);

	/* Level Frame - Show */	
	gtk_widget_show(lf.frame);
	gtk_widget_show(lf.vbox);
	gtk_widget_show(GTK_WIDGET(lf.level_list_view));
	//gtk_widget_show(lf.warning_label);
	gtk_widget_show(lf.button_hbox);
	gtk_widget_show(lf.level_list_hbox);
	gtk_widget_show(lf.level_list_scrollpane);
	gtk_widget_show(lf.add_button);
	gtk_widget_show(lf.remove_button);
	
	/* Difficulty frame and radio buttons - Constructor */
	if(gui->game->state == STATE_STOPPED)
		difficulty_frame = gtk_frame_new(_("Difficulty"));
	else
		difficulty_frame = gtk_frame_new(_("Difficulty (next game)"));

	difficulty_vbox = gtk_vbox_new(FALSE, 0);
	difficulty_easy_radio = gtk_radio_button_new_with_label(NULL,
			_("Easy"));
	difficulty_medium_radio = gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(difficulty_easy_radio), _("Medium"));
	difficulty_hard_radio = gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(difficulty_easy_radio), _("Hard"));

	/* Difficulty frame and radio buttons - Settings/Defaults */
	gtk_frame_set_shadow_type(GTK_FRAME(difficulty_frame), GTK_SHADOW_ETCHED_IN);
	switch(newflags->next_game_difficulty) {
		case DIFFICULTY_EASY :
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(difficulty_easy_radio), TRUE);
			break;
		case DIFFICULTY_MEDIUM :
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(difficulty_medium_radio), TRUE);
			break;
		case DIFFICULTY_HARD :
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(difficulty_hard_radio), TRUE);
			break;
		default :
			g_assert_not_reached();
	}

	/* Difficulty frame and radio buttons - Signals */
	g_signal_connect(GTK_OBJECT(difficulty_easy_radio), "toggled",
			GTK_SIGNAL_FUNC(cb_diff), (gpointer) DIFFICULTY_EASY);
	g_signal_connect(GTK_OBJECT(difficulty_medium_radio), "toggled",
			GTK_SIGNAL_FUNC(cb_diff), (gpointer) DIFFICULTY_MEDIUM);
	g_signal_connect(GTK_OBJECT(difficulty_hard_radio), "toggled",
			GTK_SIGNAL_FUNC(cb_diff), (gpointer) DIFFICULTY_HARD);

	/* Difficulty frame and radio buttons - Packing */
	gtk_box_pack_start_defaults(GTK_BOX(difficulty_vbox),
			difficulty_easy_radio);
	gtk_box_pack_start_defaults(GTK_BOX(difficulty_vbox),
			difficulty_medium_radio);
	gtk_box_pack_start_defaults(GTK_BOX(difficulty_vbox),
			difficulty_hard_radio);
	gtk_container_add(GTK_CONTAINER(difficulty_frame), difficulty_vbox);
	gtk_box_pack_start(GTK_BOX(gamevbox2), difficulty_frame, TRUE, TRUE, GNOME_PAD);

	/* Difficulty frame and radio buttons - Show */
	gtk_widget_show(difficulty_frame);
	gtk_widget_show(difficulty_vbox);
	gtk_widget_show(difficulty_easy_radio);
	gtk_widget_show(difficulty_medium_radio);
	gtk_widget_show(difficulty_hard_radio);

	/* Automatic Pause Frame - Constructor */
	pause_frame = gtk_frame_new(_("Automatic Pause"));
	pause_vbox = gtk_vbox_new(FALSE, 0);
	pause_on_focus_check = gtk_check_button_new_with_label(_("When we lose keyboard focus"));
	pause_on_pointer_check = gtk_check_button_new_with_label(_("When the mouse pointer leaves the play window"));
	pause_on_pref_check = gtk_check_button_new_with_label(_("When the preferences dialog is active"));

	/* Automatic Pause Frame - Settings */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pause_on_focus_check), newflags->pause_on_focus);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pause_on_pref_check), newflags->pause_on_pref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pause_on_pointer_check), newflags->pause_on_pointer);

	/* Automatic Pause Frame - Signals */
	g_signal_connect(GTK_OBJECT(pause_on_focus_check), "toggled", GTK_SIGNAL_FUNC(cb_checkbox_toggled), &newflags->pause_on_focus);
	g_signal_connect(GTK_OBJECT(pause_on_pointer_check), "toggled", GTK_SIGNAL_FUNC(cb_checkbox_toggled), &newflags->pause_on_pointer);
	g_signal_connect(GTK_OBJECT(pause_on_pref_check), "toggled", GTK_SIGNAL_FUNC(cb_checkbox_toggled), &newflags->pause_on_pref);

	/* Automatic Pause Frame - Packing */
	gtk_box_pack_start_defaults(GTK_BOX(pause_vbox), pause_on_focus_check);
	gtk_box_pack_start_defaults(GTK_BOX(pause_vbox), pause_on_pointer_check);
	gtk_box_pack_start_defaults(GTK_BOX(pause_vbox), pause_on_pref_check);
	gtk_container_add(GTK_CONTAINER(pause_frame), pause_vbox);
	gtk_box_pack_start(GTK_BOX(gamevbox2), pause_frame, TRUE, FALSE, GNOME_PAD);

	/* Automatic Pause Frame - Show */
	gtk_widget_show(pause_frame);
	gtk_widget_show(pause_vbox);
	gtk_widget_show(pause_on_pointer_check);
	gtk_widget_show(pause_on_focus_check);
	gtk_widget_show(pause_on_pref_check);

	/* Hide pointer */
	hide_pointer_check = gtk_check_button_new_with_label(
			_("Hide the pointer"));
	gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(hide_pointer_check),
			newflags->hide_pointer);
	gtk_box_pack_start(GTK_BOX(gamevbox2), hide_pointer_check,
			TRUE, FALSE, GNOME_PAD);
	gtk_widget_show(hide_pointer_check);
	g_signal_connect(GTK_OBJECT(hide_pointer_check), "toggled",
			GTK_SIGNAL_FUNC(cb_checkbox_toggled),
			&newflags->hide_pointer);
	
	/* Bounce Entropy - Constructor */
	bounce_entropy_label = gtk_label_new(_("Bounce Entropy (Experimental): "));
	bounce_entropy_hbox = gtk_hbox_new(FALSE, GNOME_PAD);
	bounce_entropy_adjustment = gtk_adjustment_new((gfloat)
			newflags->bounce_entropy, (gfloat) MIN_BOUNCE_ENTROPY,
			(gfloat) MAX_BOUNCE_ENTROPY, 1.0, 5.0, 1.0);
	bounce_entropy_sbutton = gtk_spin_button_new(
			GTK_ADJUSTMENT(bounce_entropy_adjustment), 1.0, 0);

	/* Bounce Entropy - Settings */
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(bounce_entropy_sbutton), 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(bounce_entropy_sbutton),
			FALSE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(bounce_entropy_sbutton),
			TRUE);

	/* Bounce Entropy - Signals */
	g_signal_connect(GTK_OBJECT(bounce_entropy_adjustment),
			"value_changed", GTK_SIGNAL_FUNC(cb_bounce_entropy),
			bounce_entropy_sbutton);

	/* Bounce Entropy - Packing */
	gtk_box_pack_start_defaults(GTK_BOX(bounce_entropy_hbox),
			bounce_entropy_label);
	gtk_box_pack_start_defaults(GTK_BOX(bounce_entropy_hbox),
			bounce_entropy_sbutton);
	gtk_box_pack_start(GTK_BOX(gamevbox2), bounce_entropy_hbox, TRUE,
			FALSE, GNOME_PAD);

	/* Bounce Entropy - Show */
	gtk_widget_show(bounce_entropy_label);
	gtk_widget_show(bounce_entropy_hbox);
	gtk_widget_show(bounce_entropy_sbutton);

	/* Add the game vbox to the dialog */
	gtk_notebook_append_page(GTK_NOTEBOOK(window_notebook),
			gamehbox, gametablabel);
}

static void init_control_page(GuiInfo *gui, GtkNotebook *window_notebook) {
	GtkWidget *controlhbox;
	GtkWidget *controlvbox1;
	GtkWidget *controlvbox2;
	GtkWidget *controltablabel;

	GtkWidget *bat_speed_label;
	GtkWidget *bat_speed_hbox;
	GtkObject *bat_speed_adjustment;
	GtkWidget *bat_speed_sbutton;

	GtkWidget *method_frame;
	GtkWidget *method_vbox;
	GtkWidget *method_mouse_radio;
	GtkWidget *method_keyboard_radio;

	GtkWidget *keyb_frame;
	GtkWidget *keyb_vbox;

	GtkWidget *keyb_left_hbox;
	GtkWidget *keyb_left_label;
	GtkWidget *keyb_left_entry;

	GtkWidget *keyb_right_hbox;
	GtkWidget *keyb_right_label;
	GtkWidget *keyb_right_entry;

	GtkWidget *keyb_fire1_hbox;
	GtkWidget *keyb_fire1_label;
	GtkWidget *keyb_fire1_entry;

	GtkWidget *keyb_fire2_hbox;
	GtkWidget *keyb_fire2_label;
	GtkWidget *keyb_fire2_entry;

	/* Init the vbox */
	controlhbox = gtk_hbox_new(TRUE, GNOME_PAD);
	controlvbox1 = gtk_vbox_new(TRUE, GNOME_PAD);
	controlvbox2 = gtk_vbox_new(TRUE, GNOME_PAD);
	controltablabel = gtk_label_new(_("Control"));
	gtk_box_pack_start(GTK_BOX(controlhbox), controlvbox1, TRUE, TRUE, GNOME_PAD);
	gtk_box_pack_start(GTK_BOX(controlhbox), controlvbox2, TRUE, TRUE, GNOME_PAD);
	gtk_widget_show(controlhbox);
	gtk_widget_show(controlvbox1);
	gtk_widget_show(controlvbox2);

        /* Method Frame and Radio Buttons */
        method_frame = gtk_frame_new(_("Method"));
        gtk_frame_set_shadow_type(GTK_FRAME(method_frame), GTK_SHADOW_ETCHED_IN);
        method_vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(method_frame), method_vbox);
        method_keyboard_radio = gtk_radio_button_new_with_label(NULL, _("Keyboard"));
        method_mouse_radio = gtk_radio_button_new_with_label_from_widget(
                        GTK_RADIO_BUTTON(method_keyboard_radio), _("Mouse"));
        gtk_box_pack_start_defaults(GTK_BOX(method_vbox),
                        method_keyboard_radio);
        gtk_box_pack_start_defaults(GTK_BOX(method_vbox),
                        method_mouse_radio);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(method_mouse_radio),
                        newflags->mouse_control);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(method_keyboard_radio),
                        newflags->keyboard_control);
        gtk_box_pack_start(GTK_BOX(controlvbox1), method_frame, TRUE, TRUE, GNOME_PAD);
        gtk_widget_show(method_frame);
        gtk_widget_show(method_vbox);
        gtk_widget_show(method_keyboard_radio);
        gtk_widget_show(method_mouse_radio);
        g_signal_connect(GTK_OBJECT(method_keyboard_radio), "toggled",
                        GTK_SIGNAL_FUNC(cb_ctrl_key), NULL);
        g_signal_connect(GTK_OBJECT(method_mouse_radio), "toggled",
                        GTK_SIGNAL_FUNC(cb_ctrl_mouse), NULL);

        /* Bat speed */
        bat_speed_label = gtk_label_new(_("Bat speed: "));
        bat_speed_hbox = gtk_hbox_new(FALSE, GNOME_PAD);
        bat_speed_adjustment = gtk_adjustment_new((gfloat) newflags->bat_speed,
                        (gfloat) MIN_BATSPEED, (gfloat) MAX_BATSPEED,
                        1.0, 5.0, 1.0);
        bat_speed_sbutton = gtk_spin_button_new(
                        GTK_ADJUSTMENT(bat_speed_adjustment), 1.0, 0);
        gtk_spin_button_set_digits(GTK_SPIN_BUTTON(bat_speed_sbutton), 0);
        gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(bat_speed_sbutton), FALSE);
        gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(bat_speed_sbutton), TRUE);
        gtk_box_pack_start_defaults(GTK_BOX(bat_speed_hbox), bat_speed_label);
        gtk_box_pack_start_defaults(GTK_BOX(bat_speed_hbox), bat_speed_sbutton);
        gtk_box_pack_start(GTK_BOX(controlvbox1), bat_speed_hbox, TRUE, FALSE, GNOME_PAD);
        gtk_widget_show(bat_speed_label);
        gtk_widget_show(bat_speed_hbox);
        gtk_widget_show(bat_speed_sbutton);
        g_signal_connect(GTK_OBJECT(bat_speed_adjustment), "value_changed",
                        GTK_SIGNAL_FUNC(cb_bat_speed), bat_speed_sbutton);

	/* Keybindings Frame */
	keyb_frame = gtk_frame_new(_("Keybindings"));
	gtk_frame_set_shadow_type(GTK_FRAME(keyb_frame), GTK_SHADOW_ETCHED_IN);
	keyb_vbox =  gtk_vbox_new(TRUE, GNOME_PAD);
	gtk_container_add(GTK_CONTAINER(keyb_frame), keyb_vbox);
	gtk_box_pack_start(GTK_BOX(controlvbox2), keyb_frame, TRUE, TRUE, GNOME_PAD);
	gtk_widget_show(keyb_vbox);
	gtk_widget_show(keyb_frame);

	/* KB: Left */
	keyb_left_hbox = gtk_hbox_new(FALSE, GNOME_PAD);
	keyb_left_label = gtk_label_new(_("Left:"));
	gtk_misc_set_alignment(GTK_MISC(keyb_left_label), 0.0, 0.5);
	keyb_left_entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(keyb_left_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(keyb_left_entry),
			gdk_keyval_name(newflags->left_key));
	gtk_box_pack_start_defaults(GTK_BOX(keyb_left_hbox), keyb_left_label);
	gtk_box_pack_start_defaults(GTK_BOX(keyb_left_hbox), keyb_left_entry);
	gtk_box_pack_start(GTK_BOX(keyb_vbox), keyb_left_hbox, TRUE, TRUE, GNOME_PAD);
	gtk_widget_show(keyb_left_label);
	gtk_widget_show(keyb_left_entry);
	gtk_widget_show(keyb_left_hbox);
	g_signal_connect(GTK_OBJECT(keyb_left_entry), "key_press_event",
			GTK_SIGNAL_FUNC(cb_key_left), NULL);

	/* KB: Right */
	keyb_right_hbox = gtk_hbox_new(FALSE, GNOME_PAD);
	keyb_right_label = gtk_label_new(_("Right:"));
	gtk_misc_set_alignment(GTK_MISC(keyb_right_label), 0.0, 0.5);
	keyb_right_entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(keyb_right_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(keyb_right_entry),
			gdk_keyval_name(newflags->right_key));
	gtk_box_pack_start_defaults(GTK_BOX(keyb_right_hbox), keyb_right_label);
	gtk_box_pack_start_defaults(GTK_BOX(keyb_right_hbox), keyb_right_entry);
	gtk_box_pack_start(GTK_BOX(keyb_vbox), keyb_right_hbox, TRUE, TRUE, GNOME_PAD);
	gtk_widget_show(keyb_right_label);
	gtk_widget_show(keyb_right_entry);
	gtk_widget_show(keyb_right_hbox);
	g_signal_connect(GTK_OBJECT(keyb_right_entry), "key_press_event",
			GTK_SIGNAL_FUNC(cb_key_right), NULL);

	/* KB: Fire1 */
	keyb_fire1_hbox = gtk_hbox_new(FALSE, GNOME_PAD);
	keyb_fire1_label = gtk_label_new(_("Fire One:"));
	gtk_misc_set_alignment(GTK_MISC(keyb_fire1_label), 0.0, 0.5);
	keyb_fire1_entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(keyb_fire1_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(keyb_fire1_entry),
			gdk_keyval_name(newflags->fire1_key));
	gtk_box_pack_start_defaults(GTK_BOX(keyb_fire1_hbox), keyb_fire1_label);
	gtk_box_pack_start_defaults(GTK_BOX(keyb_fire1_hbox), keyb_fire1_entry);
	gtk_box_pack_start(GTK_BOX(keyb_vbox), keyb_fire1_hbox, TRUE, TRUE, GNOME_PAD);
	gtk_widget_show(keyb_fire1_label);
	gtk_widget_show(keyb_fire1_entry);
	gtk_widget_show(keyb_fire1_hbox);
	g_signal_connect(GTK_OBJECT(keyb_fire1_entry), "key_press_event",
			GTK_SIGNAL_FUNC(cb_key_fire1), NULL);

	/* KB: Fire2 */
	keyb_fire2_hbox = gtk_hbox_new(FALSE, GNOME_PAD);
	keyb_fire2_label = gtk_label_new(_("Fire Two:"));
	gtk_misc_set_alignment(GTK_MISC(keyb_fire2_label), 0.0, 0.5);
	keyb_fire2_entry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(keyb_fire2_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(keyb_fire2_entry),
			gdk_keyval_name(newflags->fire2_key));
	gtk_box_pack_start_defaults(GTK_BOX(keyb_fire2_hbox), keyb_fire2_label);
	gtk_box_pack_start_defaults(GTK_BOX(keyb_fire2_hbox), keyb_fire2_entry);
	gtk_box_pack_start(GTK_BOX(keyb_vbox), keyb_fire2_hbox, TRUE, TRUE, GNOME_PAD);
	gtk_widget_show(keyb_fire2_label);
	gtk_widget_show(keyb_fire2_entry);
	gtk_widget_show(keyb_fire2_hbox);
	g_signal_connect(GTK_OBJECT(keyb_fire2_entry), "key_press_event",
			GTK_SIGNAL_FUNC(cb_key_fire2), NULL);

	/* Add it to the dialog */
	gtk_notebook_append_page(GTK_NOTEBOOK(window_notebook),
			controlhbox, controltablabel);
}

/*
 * Keybinding callbacks
 */
static void cb_key_left(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	if(!dialog)
		return;

	gtk_entry_set_text(GTK_ENTRY(widget), gdk_keyval_name(event->keyval));
	newflags->left_key = event->keyval;

	set_flags_changed(TRUE);
}

static void cb_key_right(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	if(!dialog)
		return;

	gtk_entry_set_text(GTK_ENTRY(widget), gdk_keyval_name(event->keyval));
	newflags->right_key = event->keyval;

	set_flags_changed(TRUE);
}

static void cb_key_fire1(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	if(!dialog)
		return;

	gtk_entry_set_text(GTK_ENTRY(widget), gdk_keyval_name(event->keyval));
	newflags->fire1_key = event->keyval;

	set_flags_changed(TRUE);
}

static void cb_key_fire2(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	if(!dialog)
		return;

	gtk_entry_set_text(GTK_ENTRY(widget), gdk_keyval_name(event->keyval));
	newflags->fire2_key = event->keyval;

	set_flags_changed(TRUE);
}

/*
 * Difficulty callbacks
 */
static void cb_diff(GtkWidget *widget, gpointer data) {
	newflags->next_game_difficulty = (Difficulty) data;
	set_flags_changed(TRUE);
}

/*
 * Method callbacks
 */
static void cb_ctrl_key(GtkWidget *widget, gpointer data) {
	newflags->mouse_control = FALSE;
	newflags->keyboard_control = TRUE;
	set_flags_changed(TRUE);
}
static void cb_ctrl_mouse(GtkWidget *widget, gpointer data) {
	newflags->mouse_control = TRUE;
	newflags->keyboard_control = FALSE;
	set_flags_changed(TRUE);
}

/*
 * Other option callbacks
 */

static void cb_checkbox_toggled(GtkWidget *w, gboolean *flag) {
	*flag = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	set_flags_changed(TRUE);
}

static void cb_bat_speed(GtkWidget *widget, gpointer data) {
	newflags->bat_speed = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(data));
	set_flags_changed(TRUE);
}

static void cb_bounce_entropy(GtkWidget *widget, gpointer data) {
	newflags->bounce_entropy = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(data));
	set_flags_changed(TRUE);
}

static void populate_level_list(LevelFrame *lf, GList *filenames) {
	GList *curr;
	gchar *data;
		
	for(curr = filenames; curr; curr = g_list_next(curr)) {
		data = (gchar *) curr->data;
		gtk_list_store_append(lf->level_list_store, &(lf->level_list_iter));
		gtk_list_store_set(lf->level_list_store, &(lf->level_list_iter), 0, data, -1);
	}
}

static void cb_level_list_changed(GtkTreeSelection *selection,
		gpointer data) {
	LevelFrame *lf = (LevelFrame *) data;
	GtkTreeModel *model;
	if(gtk_tree_selection_get_selected(selection, &model, &(lf->level_list_iter))) {
		if(lf->game->state == STATE_STOPPED) {
			gtk_widget_set_sensitive(GTK_WIDGET(lf->remove_button), TRUE);
		}

		if (lf->level_list_selection) {
			g_free(lf->level_list_selection);
			lf->level_list_selection = NULL;
		}

		gtk_tree_model_get(model, &(lf->level_list_iter), 0, &(lf->level_list_selection), -1);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(lf->remove_button), FALSE);
		lf->level_list_selection = NULL;
	}
}

static void cb_level_add(GtkWidget *widget, gpointer data) {
	LevelFrame *lf = (LevelFrame *) data;

	if(lf->game->state == STATE_STOPPED) {
		lf->fsel = gtk_file_selection_new ("Add Levelfile");

		/* Connect Signals */
		g_signal_connect(GTK_OBJECT(lf->fsel), "destroy", (GtkSignalFunc) cb_filesel_cancel, data);
		g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION (lf->fsel)->cancel_button), "clicked", (GtkSignalFunc) cb_filesel_cancel, data);
		
		g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION (lf->fsel)->ok_button), "clicked", (GtkSignalFunc) cb_filesel_ok, data);

		/* Set defaults */
		gtk_file_selection_set_filename(GTK_FILE_SELECTION(lf->fsel), "*.gbl");

		/* Show the selector */
		gtk_widget_show(lf->fsel);
	} else {
		gui_warning("Cannot add or remove levels while the game is running");
	}
	set_flags_changed(TRUE);
}

static void cb_level_remove(GtkWidget *widget, gpointer data) {
	LevelFrame *lf = (LevelFrame *) data;
	gchar *text;
	gchar *filename;

	if(lf->game->state == STATE_STOPPED) {
		g_assert(lf->level_list_selection);

		filename = leveldata_remove(lf->level_list_selection);
		remove_flags_levelfile(newflags, filename);
		g_free(filename);

		gtk_list_store_remove(lf->level_list_store, &(lf->level_list_iter));
		g_free(lf->level_list_selection);
		lf->level_list_selection = NULL;
	} else {
		gui_warning("Cannot add or remove levels while the game is running");
	}
	set_flags_changed(TRUE);
}

static void cb_filesel_ok(GtkWidget *w, gpointer data) {
	LevelFrame *lf = (LevelFrame *) data;
	gchar *filename;
	gchar *title;

	filename = (gchar *) gtk_file_selection_get_filename (GTK_FILE_SELECTION (lf->fsel));

	if(lf->game->state == STATE_STOPPED) {
		if((title = leveldata_add(filename))) {
			gtk_list_store_append(lf->level_list_store, &(lf->level_list_iter));
			gtk_list_store_set(lf->level_list_store, &(lf->level_list_iter), 0, title, -1);
			add_flags_levelfile(newflags, filename);
		}
	} else {
		gui_warning("Cannot add or remove levels while the game is running");
	}

	gtk_widget_destroy(GTK_WIDGET(lf->fsel));
	lf->fsel = NULL;
	set_flags_changed(TRUE);
}

static void cb_filesel_cancel(GtkWidget *w, gpointer data) {
	LevelFrame *lf = (LevelFrame *) data;

	gtk_widget_destroy(GTK_WIDGET(lf->fsel));
	lf->fsel = NULL;
}
