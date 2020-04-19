/* -*- Mode: C; tab-width: 4 -*- */
/* confetti --- a fading plot of sine squared */

#if 0
static const char sccsid[] = "@(#)confetti.c	5.00 2000/11/01 xlockmore";
#endif

/*-
 * Some easy plotting stuff, by Bas van Gaalen, Holland, PD
 *
 * Copyright (c) 1996 by Charles Vidal
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Revision History:
 * 01-Nov-2000: Allocation checks
 * 10-May-1997: Compatible with screensaver
 * 1996: Written by Charles Vidal based on work by Bas van Gaalen
 */

#ifdef STANDALONE
# define MODE_confetti
# define DEFAULTS	"*delay: 30000 \n" \
					"*count: 2000 \n" \
					"*cycles: 2000 \n" \
					"*ncolors: 64 \n" \
					"*fpsSolid: true \n" \
					"*ignoreRotation: True \n" \

# define BRIGHT_COLORS
# define UNIFORM_COLORS
# define release_confetti 0
# define confetti_handle_event 0
# include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
# include "xlock.h"		/* in xlockmore distribution */
#endif /* STANDALONE */

#ifdef MODE_confetti

ENTRYPOINT ModeSpecOpt confetti_opts =
{0, (XrmOptionDescRec *) NULL, 0, (argtype *) NULL, (OptionStruct *) NULL};

#ifdef USE_MODULES
ModStruct   confetti_description =
{"confetti", "init_confetti", "draw_confetti", (char *) NULL,
 "refresh_confetti", "init_confetti", "free_confetti", &confetti_opts,
 30000, 2000, 2000, 1, 64, 0.6, "",
 "Shows falling confetti", 0, NULL};

#endif

#define MINSTEPS 1
#define CONFETTI_SIZE 16
#define SHIFT2 5

typedef struct {
	XPoint	offset, delta;

	int		deltaBob;
	int		deltaFrame;
	int		indexBob;
	int		indexFrame;
	int		colors;
} Confetti;

typedef struct {
	int         count;
	int         width, height;
	Confetti   *confettis;
} confettistruct;


static confettistruct *confettis = (confettistruct *)NULL;

ENTRYPOINT void
free_confetti(ModeInfo * mi)
{
	confettistruct *fp = &confettis[MI_SCREEN(mi)];
	if (fp->confettis != NULL) {
		(void) free((void *) fp->confettis);
		fp->confettis = (Confetti *) NULL;
	}
}

#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))

static const int deltaY[] = {48,52,56,60,64,68,72,76,80,84,88,92,96,100,104,108,112,116,120,124,128};
static const int deltaBob[] = {5,7,13,17,19};
static const int deltaFrame[] = {17,8,19,5,7,13};
static const int startBob[] = {0,2,4,6,8,10,12,14,16,18,20,22,24,26,28};
static const int startFrame[] = {3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48,51,54,57};

static const int cols[] = {
		0x330000,
		0x003300,
		0x000033,
		0x333300,
		0x330033,
		0x003333,
		0x333333
};

static const int cols2[] = {
		0xff9999,
		0x99ff99,
		0x9999ff,
		0xffff99,
		0xff99ff,
		0x99ffff,
		0xffffff
};

static void gen_confetti (ModeInfo * mi)
{
	confettistruct *fp;

	if (confettis == NULL)
		return;
	fp = &confettis[MI_SCREEN(mi)];

	for (int i = 0; i < fp->count; i++) {
		fp->confettis[i].offset.x = NRAND(fp->width);
		fp->confettis[i].offset.y = NRAND(fp->height);
		fp->confettis[i].delta.x = 0;
		fp->confettis[i].delta.y = deltaY[i % ARRAYSIZE(deltaY)];
		fp->confettis[i].deltaBob = deltaBob[i % ARRAYSIZE(deltaBob)];
		fp->confettis[i].deltaFrame = deltaFrame[i % ARRAYSIZE(deltaFrame)];
		fp->confettis[i].indexBob = startBob[i % ARRAYSIZE(startBob)];
		fp->confettis[i].indexFrame = startFrame[i % ARRAYSIZE(startFrame)];
		fp->confettis[i].colors = cols[i % ARRAYSIZE(cols)];
	}
}

ENTRYPOINT void
init_confetti (ModeInfo * mi)
{
	confettistruct *fp;

	MI_INIT (mi, confettis);
	fp = &confettis[MI_SCREEN(mi)];

	fp->width = MI_WIDTH(mi);
	fp->height = MI_HEIGHT(mi);

	MI_CLEARWINDOW(mi);

	fp->count = MI_CYCLES(mi);
	if (fp->count < 100)
		fp->count = 100;

	if (fp->confettis == NULL) {
		if ((fp->confettis = (Confetti *) calloc(fp->count, sizeof (Confetti))) ==
				 NULL) {
			free_confetti(mi);
			return;
		}
	}
	gen_confetti(mi);
}

ENTRYPOINT void
draw_confetti (ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	confettistruct *fp;

	if (confettis == NULL)
		return;
	fp = &confettis[MI_SCREEN(mi)];

	MI_IS_DRAWN(mi) = True;
	XSetForeground(display, gc, MI_BLACK_PIXEL(mi));

	for (int i = 0; i < fp->count; i++) {
		XFillArc (display, window, gc,
				fp->confettis[i].offset.x,
				fp->confettis[i].offset.y,
				CONFETTI_SIZE, CONFETTI_SIZE,
				0, 360*64);
	}

	// Update positions
	for (int i = 0; i < fp->count; i++) {
		if ((fp->confettis[i].offset.y += fp->confettis[i].delta.y) > (fp->height << SHIFT2)) {
			fp->confettis[i].offset.y -= fp->height << SHIFT2;
		}

#if 0
		indexBob += deltaBob;

		if (indexBob >= PartConfetti.NUMBER_OF_BOBS << SHIFT2) {
			indexBob -= PartConfetti.NUMBER_OF_BOBS << SHIFT2;
		}

		indexFrame += deltaFrame;

		if (indexFrame >= PartConfetti.NUMBER_OF_BOBFRAMES << SHIFT2) {
			indexFrame -= PartConfetti.NUMBER_OF_BOBFRAMES << SHIFT2;
		}
#endif
		XSetForeground(display, gc, fp->confettis[i].colors);

		XFillArc (display, window, gc,
				fp->confettis[i].offset.x,
				fp->confettis[i].offset.y,
				CONFETTI_SIZE, CONFETTI_SIZE,
				0, 360*64);
	}
}

ENTRYPOINT void
reshape_confetti(ModeInfo * mi, int width, int height)
{
	confettistruct *fp = &confettis[MI_SCREEN(mi)];
	fp->width  = width;
	fp->height = height;

	gen_confetti(mi);
}

#ifndef STANDALONE
ENTRYPOINT void
refresh_confetti (ModeInfo * mi)
{
	MI_CLEARWINDOW(mi);
}
#endif

XSCREENSAVER_MODULE ("confetti", confetti)

#endif /* MODE_confetti */
