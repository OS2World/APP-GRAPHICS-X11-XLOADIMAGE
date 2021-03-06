/* pbm.c:
 *
 * portable bit map (pbm) format images
 *
 * jim frost 09.27.89
 *
 * patched by W. David Higgins (wdh@mkt.csd.harris.com) to support
 * raw-format PBM files.
 *
 * patched by Ian MacPhedran (macphed@dvinci.usask.ca) to support
 * PGM and PPM files (03-July-1990)
 */

#include "image.h"
#include "pbm.h"

/* SUPPRESS 558 */

static int          IntTable[256];
static unsigned int Initialized= 0;

#define NOTINT  -1
#define COMMENT -2
#define SPACE   -3
#define NEWLINE -4

#define BADREAD    0 /* read error */
#define NOTPBM     1 /* not a pbm file */
#define PBMNORMAL  2 /* pbm normal type file */
#define PBMCOMPACT 3 /* pbm compacty type file */
#define PBMRAWBITS 4 /* pbm raw bits type file */
#define PGMNORMAL  5 /* pgm normal type file */
#define PGMRAWBITS 6 /* pgm raw bytes type file */
#define PPMNORMAL  7 /* ppm normal type file */
#define PPMRAWBITS 8 /* ppm raw bytes type file */

static void initializeTable()
{ unsigned int a;

  for (a= 0; a < 256; a++)
    IntTable[a]= NOTINT;
  IntTable['#']= COMMENT;
  IntTable['\n']= NEWLINE;
  IntTable['\r']= IntTable['\t']= IntTable[' ']= SPACE;
  IntTable['0']= 0;
  IntTable['1']= 1;
  IntTable['2']= 2;
  IntTable['3']= 3;
  IntTable['4']= 4;
  IntTable['5']= 5;
  IntTable['6']= 6;
  IntTable['7']= 7;
  IntTable['8']= 8;
  IntTable['9']= 9;
  Initialized= 1;
}

static int pbmReadChar(zf)
     ZFILE *zf;
{ int c;

  if ((c= zgetc(zf)) == EOF) {
    zclose(zf);
    return(-1);
  }
  if (IntTable[c] == COMMENT)
    do {
      if ((c= zgetc(zf)) == EOF)
	return(-1);
    } while (IntTable[c] != NEWLINE);
  return(c);
}

static int pbmReadInt(zf)
     ZFILE *zf;
{ int c, value;

  for (;;) {
    c= pbmReadChar(zf);
    if (c < 0)
      return(-1);
    if (IntTable[c] >= 0)
      break;
  };

  value= IntTable[c];
  for (;;) {
    c= pbmReadChar(zf);
    if (c < 0)
      return(-1);
    if (IntTable[c] < 0)
      return(value);
    value= (value * 10) + IntTable[c];
  }
}

static int isPBM(zf, name, width, height, maxval, verbose)
     ZFILE        *zf;
     char         *name;
     int          *width, *height, *maxval;
     unsigned int  verbose;
{ byte buf[4];

  if (! Initialized)
    initializeTable();

  if (zread(zf, buf, 2) != 2)
    return(NOTPBM);
  if (memToVal((byte *)buf, 2) == memToVal((byte *)"P1", 2)) {
    if (((*width= pbmReadInt(zf)) < 0) || ((*height= pbmReadInt(zf)) < 0))
      return(NOTPBM);
    *maxval = 1;
    if (verbose)
      printf("%s is a %dx%d PBM image\n", name, *width, *height);
    return(PBMNORMAL);
  }
  if (memToVal((byte *)buf, 2) == memToVal((byte *)"P4", 2)) {
    if (((*width= pbmReadInt(zf)) < 0) || ((*height= pbmReadInt(zf)) < 0))
      return(NOTPBM);
    *maxval = 1;
    if (verbose)
      printf("%s is a %dx%d RawBits PBM image\n", name, *width, *height);
    return(PBMRAWBITS);
  }
  if (memToVal(buf, 2) == 0x2a17) {
    if (zread(zf, buf, 4) != 4)
      return(NOTPBM);
    *width= memToVal((byte *)buf, 2);
    *height= memToVal((byte *)(buf + 2), 2);
    *maxval = 1;
    if (verbose)
      printf("%s is a %dx%d Compact PBM image\n", name, *width, *height);
    return(PBMCOMPACT);
  }
  if (memToVal(buf, 2) == memToVal((byte *)"P2", 2)) {
    if (((*width= pbmReadInt(zf)) < 0) || ((*height= pbmReadInt(zf)) < 0))
      return(NOTPBM);
    *maxval = pbmReadInt(zf);
    if (verbose)
      printf("%s is a %dx%d PGM image with %d levels\n", name, *width,
	*height, (*maxval+1));
    return(PGMNORMAL);
  }
  if (memToVal(buf, 2) == memToVal((byte *)"P5", 2)) {
    if (((*width= pbmReadInt(zf)) < 0) || ((*height= pbmReadInt(zf)) < 0))
      return(NOTPBM);
    *maxval = pbmReadInt(zf);
    if (verbose)
      printf("%s is a %dx%d Raw PGM image with %d levels\n", name, *width,
	*height, (*maxval+1));
    return(PGMRAWBITS);
  }
  if (memToVal(buf, 2) == memToVal((byte *)"P3", 2)) {
    if (((*width= pbmReadInt(zf)) < 0) || ((*height= pbmReadInt(zf)) < 0))
      return(NOTPBM);
    *maxval = pbmReadInt(zf);
    if (verbose)
      printf("%s is a %dx%d PPM image with %d levels\n", name, *width,
	*height, (*maxval+1));
    return(PPMNORMAL);
  }
  if (memToVal(buf, 2) == memToVal((byte *)"P6", 2)) {
    if (((*width= pbmReadInt(zf)) < 0) || ((*height= pbmReadInt(zf)) < 0))
      return(NOTPBM);
    *maxval = pbmReadInt(zf);
    if (verbose)
      printf("%s is a %dx%d Raw PPM image with %d levels\n", name, *width,
	*height, (*maxval+1));
    return(PPMRAWBITS);
  }
  return(NOTPBM);
}

int pbmIdent(fullname, name)
     char *fullname, *name;
{ ZFILE *zf;
  int    width, height, maxval, ret;

  if (! (zf= zopen(fullname)))
    return(0);

  ret= isPBM(zf, name, &width, &height, &maxval, (unsigned int)1);
  zclose(zf);
  return(ret != NOTPBM);
}

Image *pbmLoad(fullname, name, verbose)
     char         *fullname, *name;
     unsigned int  verbose;
{ ZFILE        *zf;
  Image        *image;
  int           pbm_type;
  int           x, y;
  int           width, height, maxval, depth;
  unsigned int  linelen;
  byte          srcmask, destmask;
  byte         *destptr, *destline;
  int           src, size;
  int           red, grn, blu;

  if (! (zf= zopen(fullname)))
    return(NULL);

  pbm_type= isPBM(zf, name, &width, &height, &maxval, verbose);
  if (pbm_type == NOTPBM) {
    zclose(zf);
    return(NULL);
  }
  znocache(zf);
  switch (pbm_type) {
  case PBMNORMAL:
    image= newBitImage(width, height);
    linelen= (width / 8) + (width % 8 ? 1 : 0);
    destline= image->data;
    for (y= 0; y < height; y++) {
      destptr= destline;
      destmask= 0x80;
      for (x= 0; x < width; x++) {
	do {
	  if ((src= pbmReadChar(zf)) < 0) {
	    fprintf(stderr, "%s: Short image\n", fullname);
	    zclose(zf);
	    return(image);
	  }
	  if (IntTable[src] == NOTINT) {
	    fprintf(stderr, "%s: Bad image data\n", fullname);
	    zclose(zf);
	    return(image);
	  }
	} while (IntTable[src] < 0);
	
	switch (IntTable[src]) {
	case 1:
	  *destptr |= destmask;
	case 0:
	  if (! (destmask >>= 1)) {
	    destmask= 0x80;
	    destptr++;
	  }
	  break;
	default:
	  fprintf(stderr, "%s: Bad image data\n", fullname);
	  zclose(zf);
	  return(image);
	}
      }
      destline += linelen;
    }
    break;

  case PBMRAWBITS:
    image= newBitImage(width, height);
    destline= image->data;
    linelen= (width + 7) / 8;
    srcmask= 0;		/* force initial read */
    for (y= 0; y < height; y++) {
      destptr= destline;
      destmask= 0x80;
      if (srcmask != 0x80) {
        srcmask= 0x80;
	src= zgetc(zf);
	if (src == EOF) {
	  fprintf(stderr, "%s: Short image\n", fullname);
	  zclose(zf);
	  return(image);
	}
      }
      for (x= 0; x < width; x++) {
	if (src & srcmask)
	  *destptr |= destmask;
	if (! (destmask >>= 1)) {
	  destmask= 0x80;
	  destptr++;
	}
	if (! (srcmask >>= 1)) {
	  srcmask= 0x80;
	  src = zgetc(zf);
	  if (src == EOF) {
	    fprintf(stderr, "%s: Short image\n", fullname);
	    zclose(zf);
	    return(image);
	  }
	}
      }
      destline += linelen;
    }
    break;
 
  case PBMCOMPACT:
    image= newBitImage(width, height);
    destline= image->data;
    linelen= (width / 8) + (width % 8 ? 1 : 0);
    srcmask= 0x80;
    destmask= 0x80;
    src = zgetc(zf);
    if (src == EOF) {
      fprintf(stderr, "%s: Short image\n", fullname);
      zclose(zf);
      return(image);
    }
    for (y= 0; y < height; y++) {
      destptr= destline;
      destmask= 0x80;
      for (x= 0; x < width; x++) {
	if (src & srcmask)
	  *destptr |= destmask;
	if (! (destmask >>= 1)) {
	  destmask= 0x80;
	  destptr++;
	}
	if (! (srcmask >>= 1)) {
	  srcmask= 0x80;
	  src = zgetc(zf);
	  if (src == EOF) {
	    fprintf(stderr, "%s: Short image\n", fullname);
	    zclose(zf);
	    return(image);
	  }
	}
      }
      destline += linelen;
    }
    break;
  case PGMRAWBITS:
    depth= colorsToDepth(maxval);
    if (depth > 8)
      image = newTrueImage(width, height);
    else {
      image = newRGBImage(width, height, depth);
      for (y = 0; y <= maxval; y++)
	{ /* As in sunraster.c, use simple ramp for grey scale */
	  *(image->rgb.red + y) = PM_SCALE(y, maxval, 0xffff);
	  *(image->rgb.green + y) = PM_SCALE(y, maxval, 0xffff);
	  *(image->rgb.blue + y) = PM_SCALE(y, maxval, 0xffff);
	}
      image->rgb.used = maxval+1;
    }
    size= height * width;

    switch (image->type) {
    case IRGB:

      /* read in the image in a chunk
       */

      if (zread(zf, image->data, size) != size) {
	fprintf(stderr, "%s: Short image\n", fullname);
	zclose(zf);
	freeImage(image);
	return(NULL);
      }
      break;

    case ITRUE:
      destptr = image->data;
      for (y = 0; y < size; y++) {
	if ((src = zgetc(zf)) == EOF) {
	  fprintf(stderr, "%s: Short image\n", fullname);
	  zclose(zf);
	  freeImage(image);
	  return(NULL);
	}
	src = PM_SCALE(src, maxval, 0xff);
	*(destptr++) = src; /* red */
	*(destptr++) = src; /* green */
	*(destptr++) = src; /* blue */
      }
      break;
    }
    break;
  case PGMNORMAL:
    depth= colorsToDepth(maxval);
    if (depth > 8)
      image= newTrueImage(width, height);
    else {
      image= newRGBImage(width, height, depth);
      for (y= 0; y <= maxval; y++)
	{ /* As in sunraster.c, use simple ramp for grey scale */
	  *(image->rgb.red + y) = PM_SCALE(y, maxval, 0xffff);
	  *(image->rgb.green + y) = PM_SCALE(y, maxval, 0xffff);
	  *(image->rgb.blue + y) = PM_SCALE(y, maxval, 0xffff);
	}
      image->rgb.used = maxval+1;
    }
    destptr= image->data;
    size= height * width;
    for (y= 0; y < size; y++) {
      if ((src = pbmReadInt(zf)) < 0)
	{
	  fprintf(stderr, "%s: Short image\n", fullname);
	  zclose(zf);
	  return(image);
	}
      else {
	if (TRUEP(image)) {
	  src= PM_SCALE(src, maxval, 0xff);
	  *(destptr++) = src; /* red */
	  *(destptr++) = src; /* green */
	  *(destptr++) = src; /* blue */
	}
	else
	  *(destptr++) = src;
      }
    }
    break;

  case PPMRAWBITS:

    /* this is nice because the bit format is exactly what we want except
     * for scaling.
     */

    image= newTrueImage(width, height);
    size= height * width * 3;
    if (zread(zf, image->data, size) != size) {
      fprintf(stderr, "%s: Short image\n", fullname);
      zclose(zf);
      freeImage(image);
      return(NULL);
    }
    destptr= image->data;
    for (y= 0; y < size; y++) {
      *destptr= PM_SCALE(*destptr, maxval, 0xff);
      destptr++;
    }
    break;
  case PPMNORMAL:
    image= newTrueImage(width, height);
    size= height * width;
    destptr= image->data;
    for (y= 0; y < size; y++) {
      if (((red= pbmReadInt(zf)) == EOF) ||
	  ((grn= pbmReadInt(zf)) == EOF) ||
	  ((blu= pbmReadInt(zf)) == EOF))
	{
	  fprintf(stderr, "%s: Short image\n", fullname);
	  zclose(zf);
	  return(image);
	}
      *(destptr++)= PM_SCALE(red, maxval, 0xff);
      *(destptr++)= PM_SCALE(grn, maxval, 0xff);
      *(destptr++)= PM_SCALE(blu, maxval, 0xff);
    }
    break;
  }
  image->title= dupString(name);
  zclose(zf);
  return(image);
}

void pbmDump(image, options, file, verbose)
     Image *image;
     char *options;
     char *file;
     int verbose;
{
  FILE *f;
  byte *srcptr;
  int srclinelen;
  int x, y;
  int normal = 0; /* raw by default */
  int white, black;
  int mask;
  Pixel pixval;
  int pixlen;
  char *name, *value;

  while (getNextTypeOption(&options, &name, &value) > 0) {
    if (!strncmp("normal", name, strlen(name)))
      normal = 1;
    else if (!strncmp("raw", name, strlen(name)))
      normal = 0;
    else
      fprintf(stderr, "pbmDump: unknown option '%s'\n", name);
  }

  f = fopen(file, "w");
  if (f == NULL) {
    perror(file);
    return;
  }

  srcptr = image->data;
  switch (image->type) {
  case IBITMAP:
    if (verbose)
      printf("Dumping %sPBM image to %s.\n", (normal ? "" : "RawBits "),
	     file);
    if (image->rgb.red[1] > image->rgb.red[0]) {
      white = 1;
      black = 0;
    }
    else {
      white = 0;
      black = 1;
    }
    if (normal) {
      fprintf(f, "P1 %d %d\n", image->width, image->height);
      mask = 0x80;
      for (y = 0; y < image->height; y++) {
	for (x = 0; x < image->width; x++) {
	  fprintf(f, "%d ", (*srcptr & mask ? black : white));
	  mask >>= 1;
	  if (mask == 0) {
	    mask = 0x80;
	    srcptr++;
	  }
	}
      }
    }
    else {
      srclinelen = (image->width / 8) + (image->width % 8 ? 1 : 0);
      fprintf(f, "P4 %d %d\n", image->width, image->height);
      for (y = 0; y < image->height; y++) {
	if (black == 1) {
	  fwrite(srcptr, srclinelen, 1, f);
	  srcptr += srclinelen;
	}
	else
	  for (x = 0; x < image->width; x++)
	    fputc(*(srcptr++) ^ 0xff, f);
      }
    }
    break;
  case IRGB:
    if (verbose)
      printf("Dumping %sPPM image to %s.\n", (normal ? "" : "RawBits "),
	     file);

    /* technically we could dump this with 2-byte values but that's
     * a lot bigger and most of the readers don't support it anyway.
     */
    fprintf(f, "P%c %d %d 255\n", (normal ? '3' : '6'),
	    image->width, image->height);
    pixlen = image->pixlen;
    for (y = 0; y < image->height; y++) {
      for (x = 0; x < image->width; x++) {
	pixval = memToVal(srcptr, pixlen);
	srcptr += pixlen;
	fprintf(f, (normal ? "%d %d %d " : "%c%c%c"),
		image->rgb.red[pixval] >> 8,
		image->rgb.green[pixval] >> 8,
		image->rgb.blue[pixval] >> 8);
      }
    }
    break;
  case ITRUE:
    if (verbose)
      printf("Dumping %sPPM image to %s.\n", (normal ? "" : "RawBits "),
	     file);
    fprintf(f, "P%c %d %d 257\n", (normal ? '3' : '6'),
	    image->width, image->height);
    for (y = 0; y < image->height; y++) {
      for (x = 0; x < image->width; x++) {
	if (normal) {
	  fprintf(f, "%d %d %d ", *srcptr,
		  *(srcptr + 1),
		  *(srcptr + 2));
	  srcptr += 3;
	}
	else {
	  fputc(*(srcptr++), f);
	  fputc(*(srcptr++), f);
	  fputc(*(srcptr++), f);
	}
      }
    }
    break;
  }
  fclose(f);
}
