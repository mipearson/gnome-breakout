SUBDIRS = po src pixmaps levels
scoredir=$(localstatedir)/games

EXTRA_DIST = \
        gnome-breakout.desktop \
	gnome-breakout.png \
	TODO \
	BUGS

iconpixmapdir = $(datadir)/pixmaps
iconpixmap_DATA = gnome-breakout.png

Gamesdir = $(datadir)/gnome/apps/Games
Games_DATA = gnome-breakout.desktop

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-touch $(DESTDIR)$(scoredir)/gnome-breakout.scores
	-chown games.games $(DESTDIR)$(scoredir)/gnome-breakout.scores
	-chmod 0664 $(DESTDIR)$(scoredir)/gnome-breakout.scores
	-chgrp games $(DESTDIR)$(bindir)/gnome-breakout && chmod 2755 $(DESTDIR)$(bindir)/gnome-breakout
