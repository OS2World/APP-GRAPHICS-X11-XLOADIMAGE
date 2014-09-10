/* options.c:
 *
 * finds which option in an array an argument matches
 *
 * jim frost 10.03.89
 *
 * Copyright 1989 Jim Frost.
 * See included file "copyright.h" for complete copyright information.
 */

#include "copyright.h"
#include <stdio.h>
#include "image.h"
#include "options.h"

extern char *ProgramName;
/* options array and definitions.  If you add something to this you also
 * need to add its OptionId in options.h.
 */

static OptionArray Options[] = {

  /* global options
   */

  { "border",     BORDER,     "color", "\
Set the color used for the border around centered or placed images.", },
  { "configuration", CONFIGURATION, NULL, "\
Show the current configuration including any information loaded from\n\
the ~/.xloadimagerc configuration file.", },
#ifdef DEBUG
  { "debug",      DBUG,       NULL, "\
Turn on synchronous mode for debugging.", },
#endif /* DEBUG */
  { "default",    DEFAULT,    NULL, "\
Set the root background to the default pattern and colors.", },
  { "display",    DISPLAY,    NULL, "\
Indicate the X display you would like to use.", },
  { "dump",       DUMP,       "type[,options] filename", "\
Dump loaded image into an image file of the specified type and name.  Use\n\
the -path or -configuration options to determine the possible formats.\n\
Some image types may allow options; these are specified following a comma.\n\
If the image type retains a title, -title can be used to set or change it." },
  { "fit",        FIT,        NULL, "\
Force the image(s) to use the default colormap.", },
  { "fork",       FORK,       NULL, "\
Background automatically.  Turns on -quiet.", },
  { "fullscreen", FULLSCREEN, NULL, "\
Use the whole screen for displaying an image.  If -onroot is also specified,\n\
the image will be zoomed to fit the size of the screen.", },
  { "geometry",   GEOMETRY,   "window_geometry", "\
Specify the size of the display window.  Ignored if -fullscreen is given.\n\
If used in conjunction with -onroot, this defines the size of the base image.", },
  { "help",       HELP,       "[option ...]", "\
Give help on a particular option or series of options.  If no option is\n\
supplied, a list of available options is given.", },
  { "identify",   IDENTIFY,   NULL, "\
Identify images rather than displaying them.", },
  { "install",    INSTALL,    NULL, "\
Force colormap installation.  This option is useful for naive window managers\n\
which do not know how to handle colormap installation, but should be avoided\n\
unless necessary.", },
  { "list",       LIST,       NULL, "\
List the images along the image path.  Use `xloadimage -path' to see the\n\
current image path.", },
  { "onroot",     ONROOT,     NULL, "\
Place the image on the root window.  If used in conjunction with -fullscreen,\n\
the image will be zoomed to fit.  -border, -at, and -center also affect the\n\
results.", },
  { "path",       PATH,       NULL, "\
Display the image path and default extensions that are loaded from the\n\
.xloadimagerc file.  This option is identical to the -configuration option\n\
and is provided for backwards compatibility.", },
  { "pixmap",     PIXMAP,     NULL, "\
Force the use of a pixmap as backing store.  This may improve performance but\n\
may not work on memory-limited servers.", },
  { "private",    PRIVATE,    NULL, "\
Force the use of a private colormap.  This happens automatically if a visual\n\
other than the default is used.  This is the opposite of -fit.", },
  { "quiet",      QUIET,      NULL, "\
Turn off verbose mode.  This is the default if using -onroot or -windowid.", },
  { "shrink",      SHRINKTOFIT, NULL, "\
Shrink a images larger than screen to fit.  This is particularly useful with\n\
DEC servers that don't allow creation of windows larger than the screen.", },
  { "slideshow",  OPT_IGNORE, NULL, "\
Compatibility switch for older versions of xloadimage.  Since this is the\n\
the default for versions 3.0 and newer, it does nothing.", },
  { "supported",  SUPPORTED,  NULL, "\
Give a list of the supported image types.", },
  { "verbose",    VERBOSE,    NULL, "\
Turn on verbose mode.  This is the default if using -view.", },
  { "version",    VER_NUM,    NULL, "\
Show the version number of this version of xloadimage.", },
  { "view",       VIEW,       NULL, "\
View an image in a window.  This is the default for all but xsetbg.", },
  { "visual",     VISUAL,     NULL, "\
Force the use of a particular visual to display an image.  Normally xloadimage\n\
will attempt to pick a visual which is reasonable for the supplied image.", },
  { "windowid",   WINDOWID,   "window_id", "\
Set the background of a particular window.  This is similar to -onroot and\n\
is useful for servers which use an untagged virtual root.  The window ID\n\
should be supplied as a hexadecimal number, eg 0x40003.", },

  /* image options
   */

  { "at",         AT,         NULL, "\
Load the image onto the base image (if using -merge) or the root window (if\n\
using -onroot) at a specific location.  If used in conjunction with -onroot\n\
this forces the -fullscreen flag on.", },
  { "background", BACKGROUND, "color", "\
Set the background pixel color for a monochrome image.  See -foreground and\n\
-invert.", },
  { "brighten",   BRIGHT,     "percentage", "\
Brighten or darken the image by a percentage.  Values greater than 100 will\n\
brighten the image, values smaller than 100 will darken it.", },
  { "center",     CENTER,     NULL, "\
Center the image on the base image (if using -merge) or the root window (if\n\
using -onroot).  If used in conjunction with -onroot this forces the\n\
-fullscreen flag on.", },
  { "clip",       CLIP,       "X,Y,W,H", "\
Clip out the rectangle specified by X,Y,W,H and use that as the image.", },
  { "colors",     COLORS,     "number_of_colors", "\
Specify the maximum number of colors to be used in displaying the image.\n\
Values of 1-32768 are acceptable although low values will not look good.\n\
This is done automatically if the server cannot support the depth of the\n\
image.", },
  { "delay",      DELAY,      "seconds", "\
Set the automatic advance delay.", },
  { "dither",     DITHER,     NULL, "\
Dither the image into monochrome.  This happens automatically if sent to\n\
a monochrome display.", },
  { "foreground", FOREGROUND, "color", "\
Set the foreground pixel color for a monochrome image.  See -background and\n\
-invert.", },
  { "gamma",      GAMMA,      "value", "\
Adjust the image's colors by a gamma value.  1.0 is the default, which does\n\
nothing.  Values under 1.0 darken the image, values higher brighten it.\n\
Typically a dark image needs a value of 2.0 to 2.5.  This option is similar\n\
to -brighten but often works better.", },
  { "global",     GLOBAL,     NULL, "\
Force the next option to apply to all images rather that to the current\n\
image.", },
  { "goto",       GOTO,       "image_name", "\
Start displaying a particlar image.  This is useful for creating image display\newn\
loops.  If two images with the target name exist, the first in the argument list\n\
will be used.", },
  { "gray",       GRAY,       NULL, "\
Convert an image to grayscale.  Will \"undither\" a bitmap image.  Also\n\
called -grey.", },
  { "grey",       GRAY,       NULL, "\
See -gray.", },
  { "halftone",   HALFTONE,   NULL, "\
Dither the image into monochrome using a halftone dither.  This preserves\n\
image detail but blows the image up by sixteen times.", },
  { "idelay",     IDELAY,     NULL, "\
This option is no longer supported due to the addition of -global.  The\n\
same functionality can be had with -delay.", },
  { "invert",     INVERT,     NULL, "\
Invert a monochrome image.  This is the same as specifying `-foreground black'\n\
and `-background white'.", },
  { "merge",      MERGE,      NULL, "\
Merge this image onto the previous image.  When used in conjunction with\n\
-at, -center, and -clip you can generate collages.", },
  { "name",       NAME,       NULL, "\
Specify that the next argument is to be the name of an image.  This is\n\
useful for loading images whose names look to be options.", },
  { "newoptions", NEWOPTIONS, NULL, "\
This option is no longer supported.", },
  { "normalize",  NORMALIZE,  NULL, "\
Normalize the image.  This expands color coverage to fit the colormap as\n\
closely as possible.  It may have good effects on an image which is too\n\
bright or too dark.", },
  { "rotate",     ROTATE,     "degrees", "\
Rotate the image by 90, 180, or 270 degrees.", },
  { "smooth",     SMOOTH,     NULL, "\
Perform a smoothing convolution on the image.  This is useful for making\n\
a zoomed image look less blocky.  Multiple -smooth arguments will run\n\
the smoother multiple times.  This option can be quite slow on large images.", },
  { "tile",       TILE,       NULL, "\
Tile this image (after any merging or processing) to create a full-screen\n\
image.  This is usually used to create a background to merge other images\n\
onto.  -geometry can be used to set the size after tiling, otherwise it\n\
defaults to the size of the display.", },
  { "title",      TITLE,      "image_title", "\
Title for the image (used in title bars and with -dump).", },
  { "type",       TYPE,       "type_name", "\
Specifies the type of image so that run-time determination is unnecessary.\n\
This generally speeds up loading but is usually not necessary.  Use the\n\
-supported option to get a list of image types supported along with their\n\
type_name.", },
  { "xzoom",      XZOOM,      "percentage", "\
Zoom the image along the X axis by a percentage.  See -zoom.", },
  { "yzoom",      YZOOM,      "percentage", "\
Zoom the image along the X axis by a percentage.  See -zoom.", },
  { "zoom",       ZOOM,       NULL, "\
Zoom the image along both axes. Values smaller than 100 will reduce the\n\
size of the image, values greater than 100 will enlarge it.  See also\n\
-xzoom and -yzoom.", },
  { NULL,         OPT_NOTOPT, NULL, NULL }
};

static void listOptions()
{ int a, width;

  printf("\nThe options are:\n\n");

  width= 0;
  for (a= 0; Options[a].name; a++) {
    width += strlen(Options[a].name) + 2;
    if (width > 78) {
      printf("\n");
      width= strlen(Options[a].name) + 2;
    }
    printf("%s%s", Options[a].name, (Options[a + 1].name ? ", " : "\n\n"));
  }
}

static int helpOnOption(option)
     char *option;
{ int a, foundone;

  if (*option == '-')
    option++;
  foundone= 0;
  for (a= 0; Options[a].name; a++)
    if (!strncmp(Options[a].name, option, strlen(option))) {
      printf("Option: %s\nUsage: %s -%s %s\nDescription:\n%s\n\n",
	     Options[a].name,
	     ProgramName,
	     Options[a].name, (Options[a].args ? Options[a].args : ""),
	     Options[a].description);
      foundone= 1;
    }
  if (!foundone)
    printf("No option `%s'.\n", option);
  return(foundone);
}

static void literalMindedUser(s)
     char *s;
{
  printf("The quotes around %s are unnecessary.  You don't have to be so\n\
literal-minded!\n", s);
}

void help(option)
     char *option;
{ char buf[BUFSIZ];

  /* batch help facility
   */

  if (option) {
    if (!helpOnOption(option))
      printf("\
Type `xloadimage -help [option ...]' to get help on a particular option or\n\
`xloadimage -help' to enter the interactive help facility.\n\n");
    return;
  }

  /* interactive help facility
   */

  printf("\nXloadimage Interactive Help Facility\n\n");
  printf("\
Type `?' for a list of options, or `.' or `quit' to leave the interactive\n\
help facility.\n");
  for (;;) {
    printf("help> ");
    buf[BUFSIZ - 1]= '\0';
    if (fgets(buf, BUFSIZ - 1, stdin) == NULL) {
      printf("quit\n");
      exit(0);
    }
    while(buf[0] && buf[strlen(buf) - 1] == '\n')
      buf[strlen(buf) - 1]= '\0';

    /* help keywords
     */

    if (!strcmp(buf, "")) {
      printf("Type `?' for a list of options\n");
      continue;
    }
    if (!strcmp(buf, "?"))
      ;
    else if (!strcmp(buf, "quit") || !strcmp(buf, "."))
      exit(0);
    else if (!strcmp(buf, "`?'"))
      literalMindedUser("the question mark");
    else if (!strcmp(buf, "`quit'")) {
      literalMindedUser("quit");
      exit(0);
    }
    else if (!strcmp(buf, "`.'")) {
      literalMindedUser("the period");
      exit(0);
    }
    else if (helpOnOption(buf))
      continue;
    listOptions();
    printf("\
You may get this list again by typing `?' at the `help>' prompt, or leave\n\
the interactive help facility with `.' or `quit'.\n");
  }
}

OptionSet *newOptionSet()
{ OptionSet *optset;

  optset= (OptionSet *)lmalloc(sizeof(OptionSet));
  optset->options= NULL;
  optset->next= NULL;
  return(optset);
}

Option *newOption(type)
     OptionId type;
{ Option *opt;

  opt= (Option *)lmalloc(sizeof(Option));
  opt->type= type;
  opt->next= NULL;
  return(opt);
}

void addOption(optset, newopt)
     OptionSet *optset;
     Option    *newopt;
{ Option *opt;

  if (optset->options) {
    for (opt= optset->options; opt->next; opt= opt->next)
      /* EMPTY */
      ;
    opt->next= newopt;
  }
  else
    optset->options= newopt;
}

/* return the option structure for a typed option
 */
Option *getOption(optset, type)
     OptionSet *optset;
     OptionId   type;
{ Option *opt;

  if (optset)
    for (opt= optset->options; opt; opt= opt->next)
      if (type == opt->type)
	return(opt);
  return(NULL);
}

/* this kills all options of a particular type in a particular set
 */
void killOption(optset, type)
     OptionSet *optset;
     OptionId   type;
{ Option *opt;

  if (!optset)
    return;
  for (opt= optset->options; opt; opt= opt->next)
    if (opt->type == type)
      opt->type= OPT_IGNORE;
}

OptionId optionNumber(arg)
     char *arg;
{ int a, b;

  if ((*arg) != '-')
    return(OPT_NOTOPT);
  for (a= 0; Options[a].name; a++) {
    if (!strncmp(arg + 1, Options[a].name, strlen(arg) - 1)) {
      for (b= a + 1; Options[b].name; b++)
	if (!strncmp(arg + 1, Options[b].name, strlen(arg) - 1))
	  return(OPT_SHORTOPT);
      return(Options[a].option_id);
    }
  }
  return(OPT_BADOPT);
}

static char *optionName(type)
     OptionId type;
{ int a;

  for (a= 0; Options[a].name; a++)
    if (Options[a].option_id == type)
      return(Options[a].name);
  return("<unknown option>");
}

static void optionUsage(type)
     OptionId type;
{
  helpOnOption(optionName(type));
  exit(1);
}

static int getInteger(type, s)
     OptionId type;
     char *s;
{ unsigned int ret;  

  if (strncmp(s, "0x", 2)) {
    if (sscanf(s, "%d", &ret) == 1)
      return(ret);
  }
  else if (sscanf(s + 2, "%x", &ret) == 1)
    return(ret);
  fprintf(stderr, "Bad integer argument for the %s option\n",
	  optionName(type));
  usageHelp();
  /* NOTREACHED */
}

static float getFloat(type, s)
     char *s;
{
  float ret;

  if (sscanf(s, "%f", &ret) != 1) {
    fprintf(stderr, "Bad floating point argument for the %s option\n",
	    optionName(type));
    usageHelp();
    /* NOTREACHED */
  }
  return(ret);
}

/* process the argument list into an option array.  the first option in
 * the returned list is the set of global options.
 */
void processOptions(argc, argv, rglobal, rimage)
     int argc;
     char *argv[];
     OptionSet **rglobal;
     OptionSet **rimage;
{ OptionSet *global_options= newOptionSet();
  OptionSet *image_options, *curset;
  Option *newopt;
  int a, global_opt= 0;

  if (!strcmp(tail(argv[0]), "xview")) {
    newopt= newOption(VERBOSE);
    addOption(global_options, newopt);
  }
  else if (!strcmp(tail(argv[0]), "xsetbg")) {
    newopt= newOption(ONROOT);
    addOption(global_options, newopt);
  }
  else {
    newopt= newOption(VERBOSE);
    addOption(global_options, newopt);
  }

  image_options= curset= newOptionSet();
  for (a= 1; a < argc; a++) {
    newopt= newOption(optionNumber(argv[a]));

    switch (newopt->type) {
    case OPT_BADOPT:
      printf("%s: Bad option\n", argv[a]);
      usageHelp();
      /* NOTREACHED */

    case OPT_IGNORE:
      lfree((byte *)newopt);
      continue;

    case NAME:
      if (++a >= argc)
	optionUsage(NAME);
      /* FALLTHRU */

    case OPT_NOTOPT:
      newopt->type= NAME;
      newopt->info.name= dupString(argv[a]);
      addOption(curset, newopt);
      curset->next= newOptionSet();
      curset= curset->next;
      continue;

    case OPT_SHORTOPT:
      printf("%s: Not enough characters to identify option\n", argv[a]);
      usageHelp();
      /* NOTREACHED */

    /* process options global to everything
     */

    case BORDER:
      if (++a >= argc)
	optionUsage(BORDER);
      newopt->info.border= argv[a];
      global_opt= 1;
      break;

    case CONFIGURATION:
      showConfiguration();
      exit(0);

#ifdef DEBUG
    case DBUG:
      _Xdebug= 1;
      continue;
#endif /* DEBUG */

    case DEFAULT:
      addOption(curset, newopt);
      curset->next= newOptionSet();
      curset= curset->next;
      newopt= newOption(ONROOT); /* assume -onroot */
      addOption(global_options, newopt);
      newopt= newOption(FIT); /* assume -fit */
      global_opt= 1;
      break;

    case DISPLAY:
      if (++a >= argc)
	optionUsage(DISPLAY);
      newopt->info.display= argv[a];
      global_opt= 1;
      break;

    case DUMP:
      if (a + 2 >= argc)
	optionUsage(DUMP);
      newopt->info.dump.type= argv[++a];
      newopt->info.dump.file= argv[++a];
      global_opt= 1;
      break;

    case FIT:
      global_opt= 1;
      break;

    case FORK:
#ifdef NO_FORK
      fprintf(stderr, "%s is not supported under this operating system",
	      optionName(FORK));
      continue;
#else
      global_opt= 1;
      break;
#endif

    case FULLSCREEN:
      global_opt= 1;
      break;

    case GEOMETRY:
      if (++a >= argc)
	optionUsage(GEOMETRY);
      if (sscanf(argv[a], "%dx%d",
		 &newopt->info.geometry.w, &newopt->info.geometry.h) != 2) {
	optionUsage(GEOMETRY);
      }
      newopt->info.geometry.string= argv[a];
      global_opt= 1;
      break;

    case GLOBAL:
      global_opt= 1; /* next option will be put on global argument list */
      continue;

    case HELP:
      if (++a >= argc)
	help(NULL);
      else
	do {
	  help(argv[a++]);
	} while (a < argc); 
      exit(0);

    case IDENTIFY:
      global_opt= 1;
      break;

    case LIST:
      listImages();
      exit(0);

    case INSTALL:
      global_opt= 1;
      break;

    case ONROOT:
      addOption(global_options, newopt);
      newopt= newOption(FIT); /* assume -fit */
      global_opt= 1;
      break;

    case PIXMAP:
      global_opt= 1;
      break;

    case PRIVATE:
      global_opt= 1;
      break;

    case QUIET:
      killOption(global_options, VERBOSE);
      global_opt= 1;
      break;

    case SHRINKTOFIT:
      global_opt= 1;
      break;

    case SUPPORTED:
      supportedImageTypes();
      exit(0);

    case VERBOSE:
      global_opt= 1;
      break;

    case VER_NUM:
      version();
      exit(0);

    case VIEW:
      killOption(global_options, DUMP);
      killOption(global_options, ONROOT);
      global_opt= 1;
      break;

    case VISUAL:
      if (++a >= argc)
	optionUsage(VISUAL);
      newopt->info.visual= visualClassFromName(argv[a]);
      global_opt= 1;
      break;

    case WINDOWID:
      if (++a >= argc)
	optionUsage(WINDOWID);
      killOption(global_options, DUMP);
      killOption(global_options, VIEW);
      newopt->info.windowid= getInteger(WINDOWID, argv[a]);
      addOption(global_options, newopt);
      newopt= newOption(ONROOT);
      addOption(global_options, newopt);
      newopt= newOption(FIT);
      global_opt= 1;
      break;

    /* process options local to an image
     */

    case AT:
      if (!argv[++a])
	break;
      if (sscanf(argv[a], "%d,%d",
		 &newopt->info.at.x, &newopt->info.at.y) != 2)
	optionUsage(AT);
      break;

    case BACKGROUND:
      if (++a >= argc)
	optionUsage(BACKGROUND);
      newopt->info.background= argv[a];
      break;

    case BRIGHT:
      if (++a >= argc)
	optionUsage(BRIGHT);
      newopt->info.bright= getInteger(BRIGHT, argv[a]);
      break;

    case CENTER:
      break;

    case CLIP:
      if (++a >= argc)
	optionUsage(CLIP);
      if (sscanf(argv[a], "%d,%d,%d,%d",
		 &newopt->info.clip.x, &newopt->info.clip.y,
		 &newopt->info.clip.w, &newopt->info.clip.h) != 4)
	optionUsage(CLIP);
      break;

    case COLORS:
      if (++a >= argc)
	optionUsage(COLORS);
      newopt->info.colors= getInteger(COLORS, argv[a]);
      if (newopt->info.colors < 2) {
	printf("Argument to %s is too low (ignored)\n", optionName(COLORS));
	newopt->type= OPT_IGNORE;
      }
      else if (newopt->info.colors > 65536) {
	printf("Argument to %s is too high (ignored)\n", optionName(COLORS));
	newopt->type= OPT_IGNORE;
      }
      break;

    case IDELAY:
      fprintf(stderr, "%s has been superceded by %s (translating)\n",
	      optionName(IDELAY), optionName(DELAY));
      /* FALLTHRU */

    case DELAY:
      if (++a >= argc)
	optionUsage(DELAY);
#ifdef NO_DELAY
      fprintf(stderr, "%s is not supported on this operating system (ignored)\n",
	      optionName(DELAY));
      
      continue;
#else /* !NO_DELAY */
      newopt->info.delay= getInteger(DELAY, argv[a]);
      break;
#endif /* !NO_DELAY */

    case DITHER:
      break;

    case FOREGROUND:
      if (++a >= argc)
	optionUsage(FOREGROUND);
      newopt->info.foreground= argv[a];
      break;

    case GAMMA:
      if (++a >= argc)
	optionUsage(GAMMA);
      newopt->info.gamma= getFloat(GAMMA, argv[a]);
      break;

    case GOTO:
      if (++a >= argc)
	optionUsage(GOTO);
      newopt->info.go_to= argv[a];
      break;

    case GRAY:
      break;

    case HALFTONE:
      if (global_opt)
	killOption(global_options, DITHER);
      else
	killOption(curset, DITHER);
      break;

    case INVERT:
      killOption(curset, FOREGROUND);
      killOption(curset, BACKGROUND);
      newopt->type= FOREGROUND;
      newopt->info.foreground= "white";
      if (global_opt)
	addOption(global_options, newopt);
      else
	addOption(curset, newopt);
      newopt= newOption(BACKGROUND);
      newopt->info.background= "black";
      break;

    case MERGE:
      break;

    case NEWOPTIONS:
      fprintf(stderr, "%s is no longer supported (sorry)\n",
	      optionName(NEWOPTIONS));
      newopt->type= OPT_IGNORE;
      break;

    case NORMALIZE:
      break;

    case ROTATE:
      if (++a >= argc)
	optionUsage(ROTATE);
      newopt->info.rotate= getInteger(ROTATE, argv[a]);
      if (newopt->info.rotate % 90) {
	fprintf(stderr, "Argument to %s must be a multiple of 90 (ignored)\n",
		optionName(ROTATE));
	newopt->type= OPT_IGNORE;
      }
      else {
	while (newopt->info.rotate < 0)
	  newopt->info.rotate += 360;
	while (newopt->info.rotate > 360)
	  newopt->info.rotate -= 360;
      }
      break;

    case SMOOTH:
      break;

    case TILE:
      break;

    case TITLE:
      if (++a >= argc)
	optionUsage(TITLE);
      newopt->info.title= argv[a];
      break;

    case TYPE:
      if (++a >= argc)
	optionUsage(TYPE);
      newopt->info.type= argv[a];
      break;

    case XZOOM:
      if (++a >= argc)
	optionUsage(XZOOM);
      newopt->type= ZOOM;
      newopt->info.zoom.x= getInteger(XZOOM, argv[a]);
      newopt->info.zoom.y= 0;
      break;

    case YZOOM:
      if (++a >= argc)
	optionUsage(YZOOM);
      newopt->type= ZOOM;
      newopt->info.zoom.x= 0;
      newopt->info.zoom.y= getInteger(YZOOM, argv[a]);
      break;

    case ZOOM:
      if (++a >= argc)
	optionUsage(ZOOM);
      newopt->info.zoom.x= newopt->info.zoom.y= getInteger(ZOOM, argv[a]);
      if (global_opt)
	addOption(global_options, newopt);
      else
	addOption(curset, newopt);
      newopt= newOption(YZOOM);
      newopt->info.zoom.y= getInteger(ZOOM, argv[a]);
      break;

    default:

      /* this should not happen!
       */

      printf("%s: Internal error parsing arguments\n", argv[0]);
      exit(1);
    }

    if (global_opt) {
      addOption(global_options, newopt);
      global_opt= 0;
    }
    else
      addOption(curset, newopt);
  }
  *rglobal= global_options;
  *rimage= image_options;
}

/* this is used by readers/dumpers to parse options strings of the
 * form name[=value][,...]
 */
int getNextTypeOption(opt_string, name, value)
     char **opt_string;
     char **name;
     char **value;
{
  static char option_name[BUFSIZ];
  static char option_value[BUFSIZ];
  char *p;

  /* if no string, don't get anything
   */
  if (!opt_string || !*opt_string || !**opt_string)
    return 0;

  /* look for comma, the option delimiter
   */
  p = index(*opt_string, ',');
  if (p != NULL) {
    strncpy(option_name, *opt_string, p - *opt_string);
    option_name[p - *opt_string] = '\0';
    *opt_string = p + 1; /* increment to next option */
  }
  else {
    strcpy(option_name, *opt_string);
    *opt_string += strlen(*opt_string); /* increment to end of string */
  }
  *name = option_name;

  /* look for equals sign, the start of a value
   */
  p = index(option_name, '=');
  if (p != NULL) {
    strcpy(option_value, p + 1);
    *p = '\0'; /* stomp equals sign */
    *value = option_value;
  }
  else
    *value = NULL; /* no value */
  return(1);
}
