/*
 * Utility functions
 *
 * Copyright (c) 2004 Michael Pearson <mipearson@internode.on.net>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 *
 * $Id: $
 */

#include "breakout.h"
#include "gui.h"

void gb_error(gchar *format, ...) {
	va_list ap;
	gchar *message;
	gchar *message_exit;

	va_start(ap, format);
	message = g_strdup_vprintf(format, ap);
	va_end(ap);

	message_exit = g_strdup_printf("%s\n\nProgram will now exit.", message);

	fprintf(stderr, "ERROR: %s\n", message);
	gui_error(message_exit);

	g_free(message);
	g_free(message_exit);

	//g_error(message);
	exit(1);
}

void gb_warning(gchar *format, ...) {
	gchar *message;
	va_list ap;

	va_start(ap, format);
	message = g_strdup_vprintf(format, ap);
	va_end(ap);

	g_warning(message);
	g_free(message);
}
