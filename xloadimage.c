/* xloadimage.c:
 *
 * generic image loader for X11
 *
 * jim frost 09.27.89
 *
 * Copyright 1989, 1990, 1991 Jim Frost.
 * See included file "copyright.h" for complete copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"
#ifdef VMS
#include "patchlevel."
#define NO_FORK
#else
#include "patchlevel"
#endif
#include <signal.h>

extern double atof();

char *ProgramName= "xloadimage";

/* if an image loader needs to have our display and screen, it will get
 * them from here.  this is done to keep most of the image routines
 * clean
 */

Display      *Disp= NULL;
int           Scrn= 0;
int           _Xdebug = 0;
char         *BuildDate = "18/3/96";
char         *BuildUser = "Filippo Sartori";
char         *BuildSystem = "OS/2";


/* used for the -default option.  this is the root weave bitmap with
 * the bits in the order that xloadimage likes.
 */

#define root_weave_width 4
#define root_weave_height 4
static byte root_weave_bits[] = {
  0xe0, 0xb0, 0xd0, 0x70
};

static Image *doProcessOnImage(image, option, verbose)
     Image *image;
     Option *option;
{ Image  *retimage= image;
  XColor  xcolor; /* color for foreground/background */

  switch (option->type) {
  case BACKGROUND:
    if (image->depth > 1)
      break;
    XParseColor(Disp, DefaultColormap(Disp, Scrn), option->info.background,
		&xcolor);
    image->rgb.red[0]= xcolor.red;
    image->rgb.green[0]= xcolor.green;
    image->rgb.blue[0]= xcolor.blue;
    break;

  case BRIGHT:
    brighten(image, option->info.bright, verbose);
    break;

  case CLIP:
    retimage= clip(image, option->info.clip.x, option->info.clip.y,
		   option->info.clip.w, option->info.clip.h, verbose);
    break;

  case COLORS:
    retimage= reduce(image, option->info.colors, verbose);
    break;

  case DITHER:
    retimage= dither(image, verbose);
    break;

  case FOREGROUND:
    if (image->depth > 1)
      break;
    XParseColor(Disp, DefaultColormap(Disp, Scrn), option->info.foreground,
		&xcolor);
    image->rgb.red[1]= xcolor.red;
    image->rgb.green[1]= xcolor.green;
    image->rgb.blue[1]= xcolor.blue;
    break;

  case GAMMA:
    gammacorrect(image, option->info.gamma, verbose);
    break;

  case GRAY:
    if (BITMAPP(image))
      retimage= undither(image, verbose);
    else
      gray(image, verbose);
    break;

  case HALFTONE:
    retimage= halftone(image, verbose);
    break;

  case NORMALIZE:
    retimage= normalize(image, verbose);
    break;

  case ROTATE:
    retimage= rotate(image, option->info.rotate, verbose);
    break;

  case SMOOTH:
    retimage= smooth(image, 1, verbose);
    break;

  case TITLE:
    if (image->title)
      lfree((byte *)image->title);
    image->title= dupString(option->info.title);
    break;

  case ZOOM:
    retimage= zoom(image, option->info.zoom.x, option->info.zoom.y, verbose);
    break;
  }
  return(retimage);
}

/* process a list of options on an image
 */
static Image *processImage(image, global_options, image_options)
     Image *image;
     OptionSet *global_options;
     OptionSet *image_options;
{ Option       *opt;
  Image        *tmpimage;
  unsigned int  verbose;

  verbose= (getOption(global_options, VERBOSE) != NULL);

  /* go through the global options and process them
   */
  for (opt= global_options->options; opt; opt= opt->next) {

    /* if option already exists locally for this image, ignore it
     */
    if (getOption(image_options, opt->type))
      continue;
    tmpimage= doProcessOnImage(image, opt, verbose);
    if (tmpimage != image) {
      freeImage(image);
      image= tmpimage;
    }
  }

  /* go through local options
   */
  for (opt= image_options->options; opt; opt= opt->next) {
    tmpimage= doProcessOnImage(image, opt, verbose);
    if (tmpimage != image) {
      freeImage(image);
      image= tmpimage;
    }
  }
  return(image);
}

/* the real thing
 */

main(argc, argv)
     int argc;
     char *argv[];
{ Option *opt;
  char         *dname;
  Image        *dispimage;      /* image that will be sent to the display */
  Image        *newimage;       /* new image we're loading */
  Image        *tmpimage;
  Display      *disp;           /* display we're sending to */
  int           scrn;           /* screen we're sending to */
  char         *border;         /* name of border color */
  XColor        xcolor;         /* color for border option */
  OptionSet    *global_options; /* set of global options */
  OptionSet    *image_options;  /* set of image options */
  OptionSet    *optset, *tmpset;
  Option       *dump;
  unsigned int  fullscreen;
  unsigned int  onroot;
  unsigned int  verbose;
  unsigned int  winwidth, winheight; /* geometry of image */
  unsigned int  shrinktofit;

  /* set up internal error handlers
   */

  signal(SIGSEGV, internalError);
  signal(SIGBUS, internalError);
  signal(SIGFPE, internalError);
  signal(SIGILL, internalError);
#if defined(_AIX) && defined(_IBMR2)
  /* the RS/6000 (AIX 3.1) has a new signal, SIGDANGER, which you get
   * when memory is exhausted.  since malloc() can overcommit, it's a good
   * idea to trap this one.
   */
  signal(SIGDANGER, memoryExhausted);
#endif

  ProgramName= argv[0];
  if (argc < 2)
    usage();

  /* defaults and other initial settings.  some of these depend on what
   * our name was when invoked.
   */

  loadPathsAndExts();

  processOptions(argc, argv, &global_options, &image_options);

  verbose= (getOption(global_options, VERBOSE) != NULL);

  /* if no images are specified and we're not setting the default root,
   * this invocation is a no-op
   */
  if ((image_options->next == NULL) &&
      (getOption(image_options, NAME) == NULL) &&
      (getOption(global_options, DEFAULT) == NULL)) {
    fprintf(stderr, "%s: No images were specified.\n", argv[0]);
    usageHelp();
    /* NOTREACHED */
  }

  if (getOption(global_options, IDENTIFY)) {
    for (optset= image_options; optset; optset= optset->next) {
      if (opt= getOption(optset, NAME))
	identifyImage(opt->info.name);
    }
    exit(0);
  }

  /* start talking to the display
   */

  opt= getOption(global_options, DISPLAY);
  dname= (opt ? opt->info.display : NULL);
  if (! (Disp= disp= XOpenDisplay(dname))) {
    printf("%s: Cannot open display\n", XDisplayName(dname));
    exit(1);
  }
  Scrn= scrn= DefaultScreen(disp);
  XSetErrorHandler(errorHandler);

  /* background ourselves if the user asked us to
   */

#ifndef NO_FORK
  if (getOption(global_options, FORK))
    switch(fork()) {
    case -1:
      perror("fork");
      /* FALLTHRU */
    case 0:
      break;
    default:
      exit(0);
    }
#endif /* !NO_FORK */

  dispimage= NULL;

  onroot= (getOption(global_options, ONROOT) != NULL);
  fullscreen= (getOption(global_options, FULLSCREEN) != NULL);
  shrinktofit= (getOption(global_options, SHRINKTOFIT) != NULL);
  if (opt= getOption(global_options, GEOMETRY)) {
    winwidth= opt->info.geometry.w;
    winheight= opt->info.geometry.h;
  }
  else {
    winwidth= 0;
    winheight= 0;
  }

  /* find out if we're supposed to dump this silly thing
   */
  dump= getOption(global_options, DUMP);

  if (!getOption(global_options, DEFAULT) &&
      (dump || onroot) && (winwidth || winheight ||
			   getOption(image_options, CENTER) ||
			   getOption(image_options, AT) ||
			   fullscreen)) {
    if (!winwidth)
      winwidth= DisplayWidth(disp, scrn);
    if (!winheight)
      winheight= DisplayHeight(disp, scrn);
    opt= getOption(global_options, BORDER);
    border= (opt ? opt->info.border : NULL);
    if (border)
      XParseColor(disp, DefaultColormap(disp, scrn), border, &xcolor);
    else
      xcolor.red= xcolor.green= xcolor.blue = 65535;
    if (DefaultDepth(disp, scrn) == 1) {
      dispimage= newBitImage(winwidth, winheight);
      *(dispimage->rgb.red)= xcolor.red;
      *(dispimage->rgb.green)= xcolor.green;
      *(dispimage->rgb.blue)= xcolor.blue;
      if (xcolor.red || xcolor.blue || xcolor.green) {
	*(dispimage->rgb.red + 1)= 0;
	*(dispimage->rgb.green + 1)= 0;
	*(dispimage->rgb.blue + 1)= 0;
      }
      else {
	*(dispimage->rgb.red + 1)= 65535;
	*(dispimage->rgb.green + 1)= 65535;
	*(dispimage->rgb.blue + 1)= 65535;
      }
      fill(dispimage, 0, 0, winwidth, winheight, 0);
    }
    else {
      dispimage= newTrueImage(winwidth, winheight);
      dispimage->rgb.used= 1;
      fill(dispimage, 0, 0, winwidth, winheight,
	   RGB_TO_TRUE(xcolor.red, xcolor.green, xcolor.blue));
    }
    dispimage->title= dupString("Root Image");
  }

  /* load in each named image
   */

  for (optset= image_options; optset; optset= optset->next) {
  get_another_image:

    /* handle -default option.  this creates a base image using the
     * default tile weave.
     */
    if (getOption(image_options, DEFAULT)) {
      newimage= newBitImage(root_weave_width, root_weave_height);
      bcopy(root_weave_bits, newimage->data,
	    ((root_weave_width / 8) + (root_weave_width % 8 ? 1 : 0)) *
	    root_weave_height);
    }
    else if (! (opt= getOption(optset, NAME))) {

      /* this gets post-processing accomplished for -dump and -onroot.
       */
      if (dispimage)
	dispimage= processImage(dispimage, global_options, optset);
      continue;
    }
    else if (! (newimage= loadImage(global_options, optset, opt->info.name, verbose)))
      continue;

    /* retitle the image if we were asked to
     */
    if (opt= getOption(optset, TITLE)) {
      if (newimage->title)
	lfree((byte *)newimage->title);
      newimage->title= dupString(opt->info.title);
    }

    /* if this is the first image and we're putting it on the root window
     * in fullscreen mode, set the zoom factors and
     * location to something reasonable.
     */

    if ((optset == image_options) && onroot && fullscreen &&
	!getOption(optset, ZOOM) && !getOption(optset, AT) &&
	!getOption(optset, CENTER)) {

      opt= newOption(ZOOM);
      if ((newimage->width > DisplayWidth(disp, scrn)) ||
	  (newimage->height > DisplayHeight(disp, scrn))) {
	opt->info.zoom.x= opt->info.zoom.y= 
	  (newimage->width - DisplayWidth(disp, scrn) >
	   newimage->height - DisplayHeight(disp, scrn) ?
	   (float)DisplayWidth(disp, scrn) / (float)newimage->width * 100.0 :
	   (float)DisplayHeight(disp, scrn) / (float)newimage->height * 100.0);
      }
      else {
	opt->info.zoom.x= opt->info.zoom.y=
	  (DisplayWidth(disp, scrn) - newimage->width <
	   DisplayHeight(disp, scrn) - newimage->height ?
	   (float)DisplayWidth(disp, scrn) / (float)newimage->width * 100.0 :
	   (float)DisplayHeight(disp, scrn) / (float)newimage->height * 100.0);
      }
      addOption(optset, opt);
      opt= newOption(CENTER);
      addOption(optset, opt);
    }

    if ((optset == image_options) && shrinktofit && !onroot &&
	!getOption(optset, ZOOM)) {

      opt= newOption(ZOOM);

      opt->info.zoom.x= opt->info.zoom.y= 
	(newimage->width - (DisplayWidth(disp, scrn) * 0.9) >
	 newimage->height - (DisplayHeight(disp, scrn) * 0.9) ?
	 ((float)DisplayWidth(disp, scrn) * 0.9)
	 / (float)newimage->width * 100.0 :
	 ((float)DisplayHeight(disp, scrn) * 0.9)
	 / (float)newimage->height * 100.0);
      addOption(optset, opt);
    }

    newimage= processImage(newimage, global_options, optset);

    /* handle -center
     */
    if (dispimage && getOption(optset, CENTER)) {
      tmpimage= merge(dispimage, newimage,
		      (int)(dispimage->width - newimage->width) / 2,
		      (int)(dispimage->height - newimage->height) / 2,
		      verbose);
      if (dispimage != tmpimage) {
	freeImage(dispimage);
	dispimage= tmpimage;
      }
    }

    /* merge onto previous image
     */
    else if (dispimage) {
      if (! dispimage->title)
	dispimage->title= dupString(newimage->title);

      /* handle -at
       */
      if (opt= getOption(optset, AT))
	tmpimage= merge(dispimage, newimage,
			opt->info.at.x, opt->info.at.y, verbose);
      else
	tmpimage= merge(dispimage, newimage, 0, 0, verbose);
      if (dispimage != tmpimage) {
	freeImage(dispimage);
	dispimage= tmpimage;
      }
      freeImage(newimage);
    }
    else
      dispimage= newimage;

    /* if the user asked for tiling we tile the image now
     */
    if (getOption(optset, TILE)) {
      if (!winwidth)
	winwidth= DisplayWidth(disp, scrn);
      if (!winheight)
	winheight= DisplayHeight(disp, scrn);
      dispimage= tile(newimage, 0, 0, winwidth, winheight, verbose);
    }

    /* if next image is to be merged onto this one, read it
     */
    if (dump || onroot || (getOption(optset->next, MERGE)))
      continue;

  redisplay_in_window:
    switch(imageInWindow(disp, scrn, dispimage, global_options,
			 optset, argc, argv, verbose)) {
    case '\0': /* window got nuked by someone */
      XCloseDisplay(disp);
      exit(1);
    case '\003':
    case 'q':  /* user quit */
      cleanUpWindow(disp);
      XCloseDisplay(disp);
      exit(0);
    case ' ':
    case 'n':  /* next image */
      if (opt= getOption(optset->next, GOTO)) {
	char *tag= opt->info.go_to;

	for (tmpset= image_options; tmpset; tmpset= tmpset->next) {
	  if ((opt= getOption(tmpset, NAME)) &&
	      !strcmp(tag, opt->info.name)) {
	    optset= tmpset;
	    freeImage(dispimage);
	    dispimage= NULL;
	    goto get_another_image; /* ick */
	  }
	}
	fprintf(stderr, "Target for -goto %s was not found\n", tag);
      }
      break;
    case 'p':  /* previous image */
      for (tmpset= image_options; tmpset && (tmpset->next != optset);
	   tmpset= tmpset->next)
	/* EMPTY */
	;
      if (!tmpset)
	goto redisplay_in_window; /* ick */
      optset= tmpset;
      goto get_another_image; /* ick */
    case '<':
      if ((opt = getOption(optset,ZOOM)) == NULL) {
	opt= newOption(ZOOM);
	opt->info.zoom.x= opt->info.zoom.y= 50.0;
	addOption(optset, opt);
      } else {
	opt->info.zoom.x= opt->info.zoom.x ? opt->info.zoom.x * 0.5 : 50;
	opt->info.zoom.y= opt->info.zoom.y ? opt->info.zoom.y * 0.5 : 50;
      }
      tmpimage= dispimage;
      dispimage=
	zoom(dispimage, 50, 50,
	     (getOption(global_options, VERBOSE) != NULL));
      if (tmpimage != dispimage)
	free(tmpimage);
      goto redisplay_in_window; /* ick */
    case '>':
      if ((opt = getOption(optset,ZOOM)) == NULL) {
	opt= newOption(ZOOM);
	opt->info.zoom.x= opt->info.zoom.y= 200.0;
	addOption(optset, opt);
      } else {
	opt->info.zoom.x= opt->info.zoom.x ? opt->info.zoom.x * 2.0 : 200;
	opt->info.zoom.y= opt->info.zoom.y ? opt->info.zoom.y * 2.0 : 200;
      }
      tmpimage= dispimage;
      dispimage=
	zoom(dispimage, 200, 200,
	     (getOption(global_options, VERBOSE) != NULL));
      if (tmpimage != dispimage)
	free(tmpimage);
      goto redisplay_in_window; /* ick */
    }
    freeImage(dispimage);
    dispimage= NULL;
  }

  /* dump image into a NIFF file rather than displaying
   */
  if (dump && dispimage) {
    for (optset= image_options; optset && optset->next; optset= optset->next)
      /* EMPTY */
      ;
    if (opt= getOption(optset, NAME)) {
      if (dispimage->title)
	lfree((byte *)dispimage->title);
      dispimage->title= dupString(opt->info.title);
    }
    dumpImage(dispimage, dump->info.dump.type, dump->info.dump.file, verbose);
    freeImage(dispimage);
    dispimage= NULL;
    exit(0);
  }

  /* display image on root
   */
  if (onroot && dispimage)
    imageOnRoot(disp, scrn, dispimage, global_options, verbose);

  /* shut down
   */
  XCloseDisplay(disp);
  exit(0);
}
