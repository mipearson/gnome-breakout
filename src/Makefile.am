
INCLUDES = -I$(top_srcdir) -I$(includedir) $(GNOMEUI_CFLAGS) \
	 -DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	 -DG_LOG_DOMAIN=\"gnome-breakout\" \
	 -DPIXMAPDIR=\"$(datadir)/gnome-breakout/pixmaps\" \
	 -DLEVELDIR=\"$(datadir)/gnome-breakout/levels\" \
	 -DGNOME_DISABLE_DEPRECATED \
	 -DGTK_DISABLE_DEPRECATED \
	 -DGDK_PIXBUF_DISABLE_DEPRECATED \
	 -DG_DISABLE_DEPRECATED \
         -Werror

bin_PROGRAMS = gnome-breakout

gnome_breakout_SOURCES = \
	anim.c anim.h animloc.h \
	ball.c ball.h \
	bat.c bat.h \
	block.c block.h \
	collision.c collision.h \
	flags.c flags.h \
	game.c game.h \
	gnome-breakout.c breakout.h \
	gui.c gui.h \
	gui-callbacks.c gui-callbacks.h \
	gui-preferences.c gui-preferences.h \
	leveldata.c leveldata.h \
	powerup.c powerup.h \
	util.c util.h

gnome_breakout_LDADD = $(GNOMEUI_LIBS) $(INTLLIBS)
