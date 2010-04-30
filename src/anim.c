/*
 * Functions to create and manipulate animation structures
 *
 * Copyright (c) 2000 Michael Pearson <alcaron@senet.com.au>
 *
 * This file is licensed under the GNU General Public License. See the file
 * "COPYING" for more details.
 */

#include"breakout.h"
#include"gui.h"
#include"animloc.h"
#include"anim.h"

/* Database of all the animations */
static Animation *animations;
static gint num_anims;

/* Internal functions */
static Animation create_new_animation(char *filename);

/* Create the animation database. Because this calls functions that call 
 * gdk_imlib_render, it assumes that you've pushed a visual and colormap. This
 * is currently handled in the init_canvas() function in gui.c */
void init_animations(void) {
	char *filename;
	char **animloc;
	Animation *anims;

	for(num_anims = 0; animlocations[num_anims]; num_anims++);
	animations = g_malloc(sizeof(Animation) * num_anims);

	animloc = animlocations;
	anims = animations;
	while(*animloc) {
		filename = g_strdup_printf("%s/%s", PIXMAPDIR, *animloc);
		*anims = create_new_animation(filename);
		g_assert(anims->num_frames);
		g_assert(anims->pixmaps);
		g_free(filename);
		animloc++;
		anims++;
	}
}

static Animation create_new_animation(char *filename) {
	Animation newanim;
	char *fullfilename;
	int i;

	/* Find the number of frames */
	fullfilename = g_strdup_printf("%s.%d.png", filename, 0);
	for(i = 0; g_file_test(fullfilename, G_FILE_TEST_EXISTS);) {
		i++;
		fullfilename = g_strdup_printf("%s.%d.png", filename, i);
	} 
	if(!i)
		gb_error("Cannot find animation pixmap %s", fullfilename);

	newanim.num_frames = i;

	/* Now add each frame */
	newanim.pixmaps = g_malloc(newanim.num_frames *
			sizeof(GdkPixbuf *));
	for(i = 0; i < newanim.num_frames; i++) {
		fullfilename = g_strdup_printf("%s.%d.png", filename, i);
                GError *gerror = NULL;
		newanim.pixmaps[i] = gdk_pixbuf_new_from_file(fullfilename,
                        &gerror);
		if(!newanim.pixmaps[i]) {
			gb_error("Cannot open %s: %s", fullfilename,
                                gerror->message);
		}
                /*
		gdk_imlib_render(newanim.pixmaps[i],
				newanim.pixmaps[i]->rgb_width,
				newanim.pixmaps[i]->rgb_height);
                */
	}

	/* Setup the other args */
	if(newanim.num_frames == 1)
		newanim.type = ANIM_STATIC;
	else
		newanim.type = ANIM_LOOP;
	newanim.frame_no = 0;
	newanim.canvas_item = NULL;
	
	return newanim;
}

/* Get an animation, and use its default type */
Animation get_animation(gint id) {
	g_assert(!(id >= num_anims) || !(id < 0));
	
	return animations[id];
}

/* Get an animation that doesn't, uh, animate */
Animation get_static_animation(gint id) {
	Animation newanim;
	g_assert(!(id >= num_anims) || !(id < 0));

	newanim = animations[id];
	newanim.type = ANIM_STATIC;

	return newanim;
}

/* Get an animation that plays once and stops */
Animation get_once_animation(gint id) {
	Animation newanim;
	g_assert(!(id >= num_anims) || !(id < 0));

	newanim = animations[id];
	newanim.type = ANIM_ONCE;

	return newanim;
}

/* Get an animation that repeats */
Animation get_loop_animation(gint id) {
	Animation newanim;
	g_assert(!(id >= num_anims) || !(id < 0));

	newanim = animations[id];
	newanim.type = ANIM_LOOP;

	return newanim;
}

/* Iterates an animation, and updates the canvas pixmap if necessary */
void iterate_animation(Entity *entity) {

	if(entity->animation.type == ANIM_STATIC)
		return;

	entity->animation.frame_no++;
	if(entity->animation.frame_no >= entity->animation.num_frames) {
		if(entity->animation.type == ANIM_ONCE) {
			entity->animation.type = ANIM_STATIC;
			entity->animation.frame_no = entity->animation.num_frames - 1;
		} else if(entity->animation.type == ANIM_LOOP) {
			entity->animation.frame_no = 0;
		} else {
			g_assert_not_reached();
		}
	}
	update_canvas_animation(entity);
}
