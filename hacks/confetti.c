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
#define SHIFT2 2
#define NUMBER_OF_BOBS 30
#define NUMBER_OF_BOBFRAMES 60
#define BOB_SIZE (CONFETTI_SIZE + CONFETTI_SIZE + 1)

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
	Pixmap      bobs[NUMBER_OF_BOBS][NUMBER_OF_BOBFRAMES];
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

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

static int calcXint(double radius, double alpha)
{
	return (int)(radius * cos(alpha * DEG2RAD) + 0.5);
}

static int calcYint(double radius, double alpha)
{
	return (int)(radius * sin(alpha * DEG2RAD) + 0.5);
}

static double calcAlpha(double x, double y)
{
	return atan(y / x) * RAD2DEG;
}

static double calcRadius(int x, int y)
{
	return sqrt(x * x + y * y);
}


ENTRYPOINT void
init_confetti (ModeInfo * mi)
{
	Display *display = MI_DISPLAY(mi);
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

	int faderColors[31];

	XWindowAttributes    hack_attributes;
	XGetWindowAttributes (display, MI_WINDOW(mi), &hack_attributes);

	int *bob0color[NUMBER_OF_BOBFRAMES];

	for (int i = 0; i < NUMBER_OF_BOBFRAMES; i++) {
		int r2a = i + i + i;
		int radius2 = calcXint(CONFETTI_SIZE, r2a);
		int colorindex = i >= NUMBER_OF_BOBFRAMES / 2 ? NUMBER_OF_BOBFRAMES - i : i;

		fp->bobs[0][i] = XCreatePixmap(MI_DISPLAY(mi), MI_WINDOW(mi), BOB_SIZE, BOB_SIZE, hack_attributes.depth);
		bob0color[i] = (int *)calloc(BOB_SIZE * BOB_SIZE, sizeof(int));

		for (int a = 90; a < 180 + 90; a++) {
			int x1 = calcXint(radius2, a);
			int y1 = calcYint(CONFETTI_SIZE, a);

			double r1 = calcRadius(x1, y1);
			double a1 = calcAlpha(x1, y1);
			x1 = calcXint(r1, a1);
			y1 = calcYint(r1, a1);

			XSetForeground(display, MI_GC(mi), faderColors[colorindex]);
			XDrawLine(display, fp->bobs[0][i], MI_GC(mi), -x1 + CONFETTI_SIZE, y1 + CONFETTI_SIZE, x1 + CONFETTI_SIZE, y1 + CONFETTI_SIZE);

			for (int x = -x1; x <= x1; x++) {
				int xx = x + CONFETTI_SIZE;
				int yy = (y1 + CONFETTI_SIZE) * BOB_SIZE;

				bob0color[i][xx+yy] = faderColors[colorindex];
			}
		}
	}

	double angle = 0;
	for (int b = 1; b < NUMBER_OF_BOBS; b++) {
		angle += (180 / NUMBER_OF_BOBS);
		for (int f = 0; f < NUMBER_OF_BOBFRAMES; f++) {
			fp->bobs[b][f] = XCreatePixmap(MI_DISPLAY(mi), MI_WINDOW(mi), BOB_SIZE, BOB_SIZE, hack_attributes.depth);

			for (double y = -CONFETTI_SIZE; y <= CONFETTI_SIZE; y += 0.7) {
				for (double x =- CONFETTI_SIZE; x <= CONFETTI_SIZE; x += 0.7) {
					double r = calcRadius(x, y);
					double a = calcAlpha(x, y) + angle;
					if (x < 0) {
						a += 180;
					}
					int x1 = CONFETTI_SIZE + calcXint(r, a);
					int y1 = CONFETTI_SIZE + calcYint(r, a);
					int x2 = (int)(x + 0.5) + CONFETTI_SIZE;
					int y2 = (int)(y + 0.5) + CONFETTI_SIZE;

					if (x1 >= 0 && x1 < BOB_SIZE && y1 >= 0 && y1 < BOB_SIZE) {
						XSetForeground(display, MI_GC(mi), bob0color[f][x2 + y2 * BOB_SIZE]);
						XDrawPoint(display, fp->bobs[b][f], MI_GC(mi), x1, y1);
					}
				}
			}
		}
	}

	for (int i = 0; i < NUMBER_OF_BOBFRAMES; i++) {
		free(bob0color[i]);
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
