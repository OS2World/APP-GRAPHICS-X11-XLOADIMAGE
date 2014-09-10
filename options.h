/* options.h:
 *
 * optionNumber() definitions
 *
 * jim frost 10.03.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

/* enum with the options in it.  If you add one to this you also have to
 * add its information to Options[] in options.c before it becomes available.
 */

typedef enum option_id {

  /* global options
   */

  OPT_NOTOPT= 0, OPT_BADOPT, OPT_SHORTOPT, OPT_IGNORE, BORDER, CONFIGURATION,
  DBUG, DEFAULT, DELAY, DISPLAY, DUMP, FIT, FORK, FULLSCREEN, GEOMETRY, HELP,
  IDENTIFY, INSTALL, LIST, ONROOT, PATH, PIXMAP, PRIVATE, QUIET,
  SHRINKTOFIT, SUPPORTED, VERBOSE, VER_NUM, VIEW, VISUAL, WINDOWID,

  /* local options
   */

  AT, BACKGROUND, BRIGHT, CENTER, CLIP, COLORS, DITHER, FOREGROUND,
  GAMMA, GLOBAL, GOTO, GRAY, HALFTONE, IDELAY, INVERT, MERGE, NAME,
  NEWOPTIONS, NORMALIZE, ROTATE, SMOOTH, TITLE, TILE, TYPE, UNDITHER,
  XZOOM, YZOOM, ZOOM
} OptionId;

/* option structure
 */
typedef struct option {
  enum option_id type;
  union {
    struct {
      unsigned int x, y;      /* location to load image at */
    } at;
    char         *background; /* background color for mono images */
    char         *border;     /* border color */
    unsigned int  bright;     /* brightness multiplier */
    struct {
      unsigned int x, y, w, h; /* area of image to be used */
    } clip;
    unsigned int  colors;     /* max # of colors to use for this image */
    unsigned int  delay;      /* # of seconds delay before auto pic advance */
    char         *display;    /* display name */
    struct {
      char *type; /* image type */
      char *file; /* file name */
    } dump;
    char         *foreground; /* foreground color for mono images */
    float         gamma;      /* display gamma value */
    struct {
      char *string;
      unsigned int w;
      unsigned int h;
    } geometry;
    char         *go_to;      /* label to go to */
    char         *name;       /* name of image */
    unsigned int  rotate;     /* # of degrees to rotate image */
    char         *title;      /* title of image */
    char         *type;       /* expected type of image */
    unsigned int  windowid;   /* windowid for changing window backgrounds */
    unsigned int  visual;     /* visual type to use */
    struct {
      unsigned int x, y;      /* zoom factors */
    } zoom;
  } info;
  struct option *next;
} Option;

/* image name and option structure used when processing arguments
 */
typedef struct option_set {
  Option            *options; /* image processing options */
  struct option_set *next;
} OptionSet;

/* option information array
 */
typedef struct option_array {
  char     *name;        /* name of the option minus preceeding '-' */
  OptionId  option_id;   /* OptionId of this option */
  char     *args;        /* arguments this option uses or NULL if none */
  char     *description; /* description of this option */
} OptionArray;

Option *getOption(); /* options.c */
Option *newOption();
OptionId optionNumber();
int getNextTypeOption();

/* imagetypes.c */
Image *loadImage _ArgProto((OptionSet *globalopts, OptionSet *options,
			    char *name, unsigned int verbose));
void   identifyImage _ArgProto((char *name));
