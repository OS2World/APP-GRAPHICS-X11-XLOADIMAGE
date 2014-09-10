/* root.c:
 *
 * this loads an image onto the root window.  changes to allow proper
 * freeing of previously allocated resources made by Deron Dann Johnson
 * (dj@eng.sun.com).
 *
 * jim frost 10.03.89
 *
 * Copyright 1989, 1990, 1991 Jim Frost.
 * See included file "copyright.h" for complete copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"
#include <X11/Xatom.h>

#define RETAIN_PROP_NAME	"_XSETROOT_ID"

void updateProperty(dpy, w, name, type, format, data, nelem)
     Display	*dpy;
     Window	w;
     char	*name;
     Atom	type;
     int	format;
     int	data;
     int	nelem;
{
  /* intern the property name */
  Atom	atom = XInternAtom(dpy, name, 0);

  /* create or replace the property */
  XChangeProperty(dpy, w, atom, type, format, PropModeReplace, 
		  (unsigned char *)&data, nelem);
}


/* Sets the close-down mode of the client to 'RetainPermanent'
 * so all client resources will be preserved after the client
 * exits.  Puts a property on the default root window containing
 * an XID of the client so that the resources can later be killed.
 */

static void
preserveResource(dpy, w)
     Display	*dpy;
     Window	w;
{
  /* create dummy resource */
  Pixmap pm= XCreatePixmap(dpy, w, 1, 1, 1);
	
  /* create/replace the property */
  updateProperty(dpy, w, RETAIN_PROP_NAME, XA_PIXMAP, 32, (int)pm, 1);
	
  /* retain all client resources until explicitly killed */
  XSetCloseDownMode(dpy, RetainPermanent);
}


/* Flushes any resources previously retained by the client,
 * if any exist.
 */

static void
freePrevious(dpy, w)
     Display	*dpy;
     Window	w;
{
  Pixmap *pm;			
  Atom	actual_type;		/* NOTUSED */
  int	format;
  int	nitems;
  int	bytes_after;

  /* intern the property name */
  Atom atom = XInternAtom(dpy, RETAIN_PROP_NAME, 0);

  /* look for existing resource allocation */
  if ((XGetWindowProperty(dpy, w, atom, 0, 1, 1/*delete*/,
			  AnyPropertyType, &actual_type, &format, (unsigned long *)&nitems,
			  (unsigned long *)&bytes_after, (unsigned char **)&pm) == Success) &&
      nitems == 1) 
    if ((actual_type == XA_PIXMAP) && (format == 32) &&
	(nitems == 1) && (bytes_after == 0)) {
      /* blast it away */
      XKillClient(dpy, (XID) *pm);
      XFree((char *)pm);
    }
    else if (actual_type != None) {
      fprintf(stderr,
	      "%s: warning: invalid format encountered for property %s\n",
	      RETAIN_PROP_NAME, "xloadimage");
    }
}

#if FIND_DEC_ROOTWINDOW
/* this returns the root window for DECWindows servers.  this is right
 * out of xsetroot with minor formatting changes.
 */

static Window getWmRootWindow(dpy, root)
     Display *dpy;
     Window root; 
{
  Window parent;
  Window *child;
  unsigned int nchildren;
  XWindowAttributes rootatt, childatt;
  if (!XGetWindowAttributes (dpy, root, &rootatt))
    return(0);

  if (XQueryTree (dpy, root, &root, &parent, &child, &nchildren)) {
    int i;
    for (i = 0; i < nchildren; i++) {
      if (!XGetWindowAttributes (dpy, child[i], &childatt))
	continue;
      if ((rootatt.width == childatt.width) &&
	  (rootatt.height == childatt.height))
	return child[i];
    }
    return root;
  }
  else
    return((Window)0);
}

static Window getDECRootWindow(dpy, root)
     Display *dpy;
     Window root; 
{
  Window temporary_rootW;

  if (!(temporary_rootW = getWmRootWindow(dpy, root)) ||
      !(temporary_rootW = getWmRootWindow(dpy, temporary_rootW))) {
    fprintf(stderr, "Can't find false root window on DECWINDOWS server, using true root\n");
    return(root);
  }
  return(temporary_rootW);
}
#endif /* FIND_DEC_ROOTWINDOW */

void imageOnRoot(disp, scrn, image, options, verbose)
     Display      *disp;
     int           scrn;
     Image        *image;
     OptionSet    *options;
     unsigned int  verbose;
{ Option       *opt;
  Pixmap        pixmap;
  XImageInfo   *ximageinfo;
  Atom          __SWM_VROOT = None;
  Window        root, rootReturn, parentReturn, *children;
  unsigned int  numChildren;
  int           i;
  char         *s;

  if (opt= getOption(options, WINDOWID))
    root= opt->info.windowid;
  else {
    root = RootWindow(disp, scrn);

#ifdef FIND_DEC_ROOTWINDOW
    /* look for DECWindows servers because they do strange stuff with the
     * root window.  they should be shot for that one.
     *
     * some X terminal vendors put DECWINDOWS in their vendor string to
     * get DEC X utilities to work.  to get around them, we look for
     * the string "DECWINDOWS Digital" because as of this writing DEC's
     * vendorstrings look like "DECWINDOWS DigitalEquipmentCorporation".
     */

    for (s= ServerVendor(disp); *s; s++)
      if (!strncmp(s, "DECWINDOWS Digital", 18))
	root= getDECRootWindow(disp, scrn);
#endif /* FIND_DEC_ROOTWINDOW */

    /* Added for window managers like swm and tvtwm that follow solbourne's 
     * virtual root window concept.
     */

    __SWM_VROOT = XInternAtom(disp, "__SWM_VROOT", False);
    XQueryTree(disp, root, &rootReturn, &parentReturn, &children, &numChildren);
    for(i = 0; i < numChildren; i++) {
      Atom actual_type;
      int actual_format;
      long nitems, bytesafter;
      Window *newRoot = NULL;
      
      if (XGetWindowProperty (disp, children[i], __SWM_VROOT,0,1,
			      False, XA_WINDOW, &actual_type, &actual_format,
			      (unsigned long *)&nitems, (unsigned long *)&bytesafter,
			      (unsigned char **) &newRoot) ==
	  Success && newRoot) {
	root = *newRoot;
	break;
      }
    }
  }
  freePrevious(disp, root);

  if (! (ximageinfo= imageToXImage(disp, scrn,
				   DefaultVisual(disp, scrn),
				   DefaultDepth(disp, scrn),
				   image, False, True, verbose))) {
    fprintf(stderr, "Cannot convert Image to XImage\n");
    exit(1);
  }
  if ((pixmap= ximageToPixmap(disp, root, ximageinfo)) == None) {
    printf("Cannot create background (not enough resources, sorry)\n");
    exit(1);
  }

  /* changing the root colormap is A Bad Thing, so deny it.
   */

  if (ximageinfo->cmap != DefaultColormap(disp, scrn)) {
    printf("Loading image onto root would change default colormap (sorry)\n");
    XFreePixmap(disp, pixmap);
    exit(1);
  }

  XSetWindowBackgroundPixmap(disp, root, pixmap);
  XClearWindow(disp, root);
  XFreePixmap(disp, pixmap);
  freeXImage(image, ximageinfo);
  preserveResource(disp, root);
}
