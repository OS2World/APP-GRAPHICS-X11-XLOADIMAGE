/* xloadimage.h:
 *
 * jim frost 06.21.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "image.h"
#include "options.h"

/* This struct holds the X-client side bits for a rendered image.
 */

typedef struct {
  Display  *disp;       /* destination display */
  int       scrn;       /* destination screen */
  int       depth;      /* depth of drawable we want/have */
  Drawable  drawable;   /* drawable to send image to */
  Pixel     foreground; /* foreground and background pixels for mono images */
  Pixel     background;
  Colormap  cmap;       /* colormap used for image */
  GC        gc;         /* cached gc for sending image */
  XImage   *ximage;     /* ximage structure */
} XImageInfo;

#ifndef MAXIMAGES
#define MAXIMAGES BUFSIZ /* max # of images we'll try to load at once */
#endif

/* function declarations
 */

void supportedImageTypes(); /* imagetypes.c */

char *tail(); /* misc.c */
void memoryExhausted();
void internalError();
void version();
void usage();
void goodImage();
int errorHandler();

char *expandPath(); /* path.c */
int findImage();
void listImages();
void loadPathsAndExts();
void showPath();

void imageOnRoot(); /* root.c */

void        sendXImage(); /* send.c */
XImageInfo *imageToXImage();
Pixmap      ximageToPixmap();
void        freeXImage();

Visual *getBestVisual(); /* visual.c */

int   visualClassFromName(); /* window.c */
char *nameOfVisualClass();
void cleanUpWindow();
char imageInWindow();
