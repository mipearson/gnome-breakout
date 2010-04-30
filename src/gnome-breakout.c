#include <unistd.h>
#include <time.h>
#include "breakout.h"
#include "flags.h"
#include "anim.h"
#include "gui.h"
#include "leveldata.h"

/* Internal Functions */
static void init_leveldata(Game *game);

/* Initialises the game struct, sets up gnome and i18n, sets up the animation
 * files, and starts the gui */
int main(int argc, char **argv) {
	Game game;
	gboolean show_score_warning = FALSE;

	show_score_warning = (gnome_score_init(PACKAGE) == -1);
	memset(&game, 0, sizeof(Game));

	srand((unsigned int) clock());

	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);
	gnome_program_init(PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv,
			GNOME_PARAM_NONE);
	gui_init(&game, argc, argv);
	game.flags = load_flags();
	init_leveldata(&game);

	init_animations();

	if(show_score_warning)
		gb_warning("Failed to initialise gnome_score. Is " PACKAGE " installed setgid to the games group?");

	gtk_main();

	return 0;
}

/* Loads the levelfiles into memory that are specified in game->flags, deleting 
 * the ones that won't load */
static void init_leveldata(Game *game) {
        GList *curr, *next;

        curr = game->flags->level_files;
        while(curr) {
		next = g_list_next(curr);
                if(!leveldata_add((gchar *) curr->data)) {
			remove_flags_levelfile(game->flags, curr->data);
                }
                curr = next;
        }
}
