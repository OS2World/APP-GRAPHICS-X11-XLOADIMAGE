/* image.h:
 *
 * portable image type declarations
 *
 * jim frost 10.02.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"

#include <stdio.h>

/* ANSI-C stuff
 */
#if defined(__STDC__)

#if !defined(_ArgProto)
#define _ArgProto(ARGS) ARGS
#endif

#include <stdlib.h>

#else /* !__STDC__ */

#if !defined(const) /* "const" is an ANSI thing */
#define const
#endif
#if !defined(_ArgProto)
#define _ArgProto(ARGS) ()
#endif

#endif /* !__STDC__ */

/* handle strings stuff that varies between BSD and ANSI/SYSV
 */
#if defined(IS_BSD) && !defined(__STDC__)
#include <strings.h>
#if !defined(strchr) && !defined(index)
#define strchr index
#endif
#if !defined(strrchr) && !defined(rindex)
#define strrchr rindex
#endif
#if !defined(memcpy) && !defined(bcopy)
#define memcpy(D,S,L) bcopy((char *)(S),(char *)(D),(L))
#endif
#if !defined(memset) && !defined(bzero)
/* #define memset(D,V,L) bzero(D,L) */
#endif
#else /* !IS_BSD || __STDC__ */
#include <string.h>
#if !defined(index) && !defined(strchr)
#define index strchr
#endif
#if !defined(rindex) && !defined(strrchr)
#define rindex strrchr
#endif
#if !defined(bcopy) && !defined(memcpy)
#define bcopy(S,D,L) memcpy((void *)(D),(void *)(S),(L))
#endif
#if !defined(bzero) && !defined(memset)
#define bzero(D,L) memset((void *)(D),0,(L))
#endif
#endif /* !IS_BSD || __STDC__ */

#ifdef VMS
#define R_OK 4
#define NO_UNCOMPRESS
#endif

typedef unsigned long  Pixel;     /* what X thinks a pixel is */
typedef unsigned short Intensity; /* what X thinks an RGB intensity is */
typedef unsigned char  byte;      /* byte type */

/* filter/extension pair for user-defined filters
 */
struct filter {
  char          *extension; /* extension to match */
  char          *filter;    /* filter to invoke */
  struct filter *next;
};

struct cache {
  int           len;
  char          buf[BUFSIZ];
  struct cache *next;
};

typedef struct {
  unsigned int  type;     /* ZIO file type */
  unsigned int  nocache;  /* true if caching has been disabled */
  FILE         *stream;   /* file input stream */
  char         *filename; /* filename */
  struct cache *data;     /* data cache */
  struct cache *dataptr;  /* ptr to current cache block */
  int           bufptr;   /* ptr within current cache block */
} ZFILE;

#define ZSTANDARD 0 /* standard file */
#define ZPIPE     1 /* file is a pipe (ie uncompress) */
#define ZSTDIN    2 /* file is stdin */

typedef struct rgbmap {
  unsigned int  size;       /* size of RGB map */
  unsigned int  used;       /* number of colors used in RGB map */
  unsigned int  compressed; /* image uses colormap fully */
  Intensity    *red;        /* color values in X style */
  Intensity    *green;
  Intensity    *blue;
} RGBMap;

/* image structure
 */

typedef struct {
  char         *title;  /* name of image */
  unsigned int  type;   /* type of image */
  RGBMap        rgb;    /* RGB map of image if IRGB type */
  unsigned int  width;  /* width of image in pixels */
  unsigned int  height; /* height of image in pixels */
  unsigned int  depth;  /* depth of image in bits if IRGB type */
  unsigned int  pixlen; /* length of pixel if IRGB type */
  float		gamma;	/* gamma of display the image is adjusted for */
  byte         *data;   /* data rounded to full byte for each row */
} Image;

#define IBAD    0 /* invalid image (used when freeing) */
#define IBITMAP 1 /* image is a bitmap */
#define IRGB    2 /* image is RGB */
#define ITRUE   3 /* image is true color */

#define BITMAPP(IMAGE) ((IMAGE)->type == IBITMAP)
#define RGBP(IMAGE)    ((IMAGE)->type == IRGB)
#define TRUEP(IMAGE)   ((IMAGE)->type == ITRUE)

#define TRUE_RED(PIXVAL)   (((PIXVAL) & 0xff0000) >> 16)
#define TRUE_GREEN(PIXVAL) (((PIXVAL) & 0xff00) >> 8)
#define TRUE_BLUE(PIXVAL)  ((PIXVAL) & 0xff)
#define RGB_TO_TRUE(R,G,B) \
  (((unsigned int)((R) & 0xff00) << 8) | ((unsigned int)(G) & 0xff00) | \
   ((unsigned int)(B) >> 8))

#ifdef NO_INLINE
/* only inline 1-byte transfers.  this is provided for systems whose C
 * compilers can't hash the complexity of the inlined functions.
 */

#define memToVal(PTR,LEN)    ((LEN) == 1 ? (unsigned long)(*(PTR)) : \
			      doMemToVal(PTR,LEN))
#define memToValLSB(PTR,LEN) ((LEN) == 1 ? (unsigned long)(*(PTR)) : \
			      doMemToValLSB(PTR,LEN))
#define valToMem(VAL,PTR,LEN)    ((LEN) == 1 ? \
				  (unsigned long)(*(PTR) = (byte)(VAL)) : \
				  doValToMem(VAL,PTR,LEN))
#define valToMemLSB(VAL,PTR,LEN) ((LEN) == 1 ? \
				  (unsigned long)(*(PTR) = (byte)(VAL)) : \
				  (int)doValToMemLSB(VAL,PTR,LEN))

#else /* !NO_INLINE */
/* inline these functions for speed.  these only work for {len : 1,2,3,4}.
 */

#define memToVal(PTR,LEN) \
  ((LEN) == 1 ? ((unsigned long)(*((byte *)PTR))) : \
   ((LEN) == 3 ? ((unsigned long) \
		  (*(byte *)(PTR) << 16) | \
		  (*((byte *)(PTR) + 1) << 8) | \
		  (*((byte *)(PTR) + 2))) : \
    ((LEN) == 2 ? ((unsigned long) \
		   (*(byte *)(PTR) << 8) | \
		   (*((byte *)(PTR) + 1))) : \
     ((unsigned long)((*(byte *)(PTR) << 24) | \
		      (*((byte *)(PTR) + 1) << 16) | \
		      (*((byte *)(PTR) + 2) << 8) | \
		      (*((byte *)(PTR) + 3)))))))

#define memToValLSB(PTR,LEN) \
  ((LEN) == 1 ? ((unsigned long)(*(byte *)(PTR))) : \
   ((LEN) == 3 ? ((unsigned long) \
		  (*(byte *)(PTR)) | \
		  (*((byte *)(PTR) + 1) << 8) | \
		  (*((byte *)(PTR) + 2) << 16)) : \
    ((LEN) == 2 ? ((unsigned long) \
		   (*(byte *)(PTR)) | (*((byte *)(PTR) + 1) << 8)) : \
     ((unsigned long)((*(byte *)(PTR)) | \
		      (*((byte *)(PTR) + 1) << 8) | \
		      (*((byte *)(PTR) + 2) << 16) | \
		      (*((byte *)(PTR) + 3) << 24))))))

#define valToMem(VAL,PTR,LEN) \
  ((LEN) == 1 ? (*(byte *)(PTR) = ((unsigned int)(VAL) & 0xff)) : \
   ((LEN) == 3 ? (((*(byte *)(PTR)) = ((unsigned int)(VAL) & 0xff0000) >> 16), \
		  ((*((byte *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
		  ((*((byte *)(PTR) + 2)) = ((unsigned int)(VAL) & 0xff))) : \
    ((LEN) == 2 ? (((*(byte *)(PTR)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
		   ((*((byte *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff))) : \
     (((*(byte *)(PTR)) = ((unsigned int)(VAL) & 0xff000000) >> 24), \
      ((*((byte *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff0000) >> 16), \
      ((*((byte *)(PTR) + 2)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
      ((*((byte *)(PTR) + 3)) = ((unsigned int)(VAL) & 0xff))))))

#define valToMemLSB(VAL,PTR,LEN) \
  ((LEN) == 1 ? (*(byte *)(PTR) = ((unsigned int)(VAL) & 0xff)) : \
   ((LEN) == 3 ? (((*(byte *)(PTR) + 2) = ((unsigned int)(VAL) & 0xff0000) >> 16), \
		  ((*((byte *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
		  ((*(byte *)(PTR)) = ((unsigned int)(VAL) & 0xff))) : \
    ((LEN) == 2 ? (((*((byte *)(PTR) + 1) = ((unsigned int)(VAL) & 0xff00) >> 8), \
		    ((*(byte *)(PTR)) = ((unsigned int)(VAL) & 0xff)))) : \
     (((*((byte *)(PTR) + 3)) = ((unsigned int)(VAL) & 0xff000000) >> 24), \
      ((*((byte *)(PTR) + 2)) = ((unsigned int)(VAL) & 0xff0000) >> 16), \
      ((*((byte *)(PTR) + 1)) = ((unsigned int)(VAL) & 0xff00) >> 8), \
      ((*(byte *)(PTR)) = ((unsigned int)(VAL) & 0xff))))))
#endif /* !NO_INLINE */

/* SUPPRESS 558 */

/* function declarations
 */
/* clip.c */
Image *clip _ArgProto((Image *image, unsigned int x, unsigned int y,
		unsigned int width, unsigned int height,
		unsigned int verbose));

/* bright.c */
void brighten _ArgProto((Image *image, unsigned int percent, unsigned int verbose));
void gammacorrect _ArgProto((Image *image, double disp_gam, unsigned int verbose));
void gray _ArgProto((Image *image, unsigned int verbose));
Image *normalize _ArgProto((Image *image, unsigned int verbose));

/* compress.c */
void compress _ArgProto((Image *image, unsigned int verbose));

/* dither.c */
Image *dither _ArgProto((Image *image, unsigned int verbose));

/* fill.c */
void fill _ArgProto((Image *image, unsigned int fx, unsigned int fy,
	      unsigned int fw, unsigned int fh, Pixel pixval));

/* halftone.c */
Image *halftone _ArgProto((Image *image, unsigned int verbose));

/* imagetypes.c */
void   goodImage _ArgProto((Image *image, char *where));

/* merge.c */
Image *merge _ArgProto((Image *dest, Image *src, int atx, int aty,
		 unsigned int verbose));
Image *tile _ArgProto((Image *image, int x, int y,
		unsigned int width, unsigned int height,
		unsigned int verbose));

/* misc.c */
void memoryExhausted _ArgProto((void));
char *tail _ArgProto((char *));
void usage _ArgProto((void));
int errorHandler();
char *findstr _ArgProto((char *, char *));

/* new.c */
extern unsigned long DepthToColorsTable[];
unsigned long colorsToDepth();
char  *dupString _ArgProto((char *s));
Image *newBitImage _ArgProto((unsigned int width, unsigned int height));
Image *newRGBImage _ArgProto((unsigned int width, unsigned int height,
		       unsigned int depth));
Image *newTrueImage _ArgProto((unsigned int width, unsigned int height));
void   freeImage _ArgProto((Image *image));
void   freeImageData _ArgProto((Image *image));
void   newRGBMapData _ArgProto((RGBMap *rgb, unsigned int  size));
void   freeRGBMapData _ArgProto((RGBMap *rgb));
byte  *lcalloc _ArgProto((unsigned int size));
byte  *lmalloc _ArgProto((unsigned int size));
void   lfree _ArgProto((byte *area));

#define depthToColors(n) DepthToColorsTable[((n) < 32 ? (n) : 32)]

/* reduce.c */
Image *reduce _ArgProto((Image *image, unsigned int n, unsigned int verbose));
Image *expand _ArgProto((Image *image));
Image *flatten _ArgProto((Image *image));

/* rotate.c */
Image *rotate _ArgProto((Image *image, unsigned int degrees, unsigned int verbose));

/* smooth.c */
Image *smooth _ArgProto((Image *image, int iterations, unsigned int verbose));

/* undither.c */
Image *undither _ArgProto((Image *oimage, unsigned int  verbose));

/* doMemToVal and doMemToValLSB used to be void type but some compilers
 * (particularly the 4.1.1 SunOS compiler) couldn't handle the
 * (void)(thing= value) conversion used in the macros.
 */
/* value.c */
unsigned long doMemToVal _ArgProto((byte *p, unsigned int len));
unsigned long doValToMem _ArgProto((unsigned long val, byte *p, unsigned int len));
unsigned long doMemToValLSB _ArgProto((byte *p, unsigned int len));
unsigned long doValToMemLSB _ArgProto((unsigned long val, byte *p, unsigned int len));
void          flipBits _ArgProto((byte *p, unsigned int len));

/* zio.c */
ZFILE *zopen _ArgProto((char *name));
int    zread _ArgProto((ZFILE *zf, byte *buf, unsigned int len));
int    zgetc _ArgProto((ZFILE *zf));
char  *zgets _ArgProto((byte *buf, unsigned int size, ZFILE *zf));
void   zclose _ArgProto((ZFILE *zf));
void   znocache _ArgProto((ZFILE *zf));
void   zreset _ArgProto((char *filename));

/* zoom.c */
Image *zoom _ArgProto((Image *image, unsigned int x, unsigned int y,
		       unsigned int verbose));

/* this returns the (approximate) intensity of an RGB triple
 */

#define colorIntensity(R,G,B) \
  (RedIntensity[(R) >> 8] + GreenIntensity[(G) >> 8] + BlueIntensity[(B) >> 8])

extern unsigned short RedIntensity[];
extern unsigned short GreenIntensity[];
extern unsigned short BlueIntensity[];

#ifdef DEBUG
extern int _Xdebug;
#define debug(ARGS) if (_Xdebug) printf ARGS
#else /* !DEBUG */
#define debug(ARGS)
#endif /* !DEBUG */
