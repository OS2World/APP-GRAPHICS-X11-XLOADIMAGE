/* tiff.c:
 *
 * interface file for TIFF image format
 *
 * jim frost 09.05.93
 */

#ifdef HAS_TIFF

#include "image.h"
#include "tiff/tiffio.h"

/* this structure contains all the information we care about WRT a TIFF
 * image.
 */
struct tiff_info {
  unsigned short type; /* little- or big-endian */
  int width;
  int height;
  int depth;
  unsigned short planarconfiguration;
  unsigned short photometric;
  unsigned short compression;
  unsigned short bitspersample;
  unsigned short samplesperpixel;
  unsigned short bytesperrow;
  char *title;
};

static TIFF *is_tiff(fullname, name, info)
     char *fullname;
     char *name;
     struct tiff_info *info;
{
  ZFILE *zf;
  TIFFHeader th;
  TIFF *tiff;

  zf = zopen(fullname);

  /* read TIFF header and see if it looks right
   */
  if ((zread(zf, (byte *)&th, sizeof(TIFFHeader)) == sizeof(TIFFHeader)) &&
      ((th.tiff_magic == TIFF_BIGENDIAN) ||
       (th.tiff_magic == TIFF_LITTLEENDIAN))) {

    /* definitely a tiff file
     */
    if (zf->type != ZSTANDARD) {
      printf("%s is a TIFF file, but TIFF files can't be read through pipes or filters, sorry.\n", name);
      return((TIFF *)-1);
    }

    info->type= th.tiff_magic;

    tiff = TIFFOpen(fullname, "r");
    if (!tiff)
      return(0);

    /* ok, start sucking in information about this file
     */
    /* find out gross information about this image
     */
    TIFFGetFieldDefaulted(tiff, TIFFTAG_IMAGEWIDTH, &info->width);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_IMAGELENGTH, &info->height);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_PLANARCONFIG, &info->planarconfiguration);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_PHOTOMETRIC, &info->photometric);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_COMPRESSION, &info->compression);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE, &info->bitspersample);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &info->samplesperpixel);
    info->bytesperrow= TIFFScanlineSize(tiff);

    /* get the "title" of the image
     */
    if (!TIFFGetField(tiff, TIFFTAG_DOCUMENTNAME, &info->title))
      if (!TIFFGetField(tiff, TIFFTAG_IMAGEDESCRIPTION, &info->title))
	info->title = NULL;
    /* make our own copy just to be safe
     */
    if (info->title)
      info->title = dupString(info->title);

#if 0 /* maybe it does */
    /* convert byte orders.  why the hell doesn't the TIFF stuff do this
     * for you?
     */
    switch (info->type) {
    case TIFF_BIGENDIAN:
#define convert(V) (V) = memToVal(&(V), sizeof(V))
      convert(info->width);
      convert(info->height);
      convert(info->planarconfiguration);
      convert(info->photometric);
      convert(info->compression);
      convert(info->bitspersample);
      convert(info->samplesperpixel);
#undef convert
      break;
    case TIFF_LITTLEENDIAN:
#define convert(V) (V) = memToValLSB(&(V), sizeof(V))
      convert(info->width);
      convert(info->height);
      convert(info->planarconfiguration);
      convert(info->photometric);
      convert(info->compression);
      convert(info->bitspersample);
      convert(info->samplesperpixel);
#undef convert
      break;
    }
#endif
    return(tiff);
  }

  zclose(zf);
  return(0);
}

static char *photometricName(info)
     struct tiff_info *info;
{
  static char buf[32];

  switch (info->photometric) {
  case PHOTOMETRIC_MINISBLACK:
    if (info->bitspersample > 1) {
      sprintf(buf, "%d-bit greyscale ", info->bitspersample);
      return(buf);
    }
    else
      return "white-on-black ";
  case PHOTOMETRIC_MINISWHITE:
    if (info->bitspersample > 1) {
      sprintf(buf, "%d-bit greyscale ", info->bitspersample);
      return(buf);
    }
    else
      return "black-on-white ";
  case PHOTOMETRIC_RGB:
    return "RGB ";
  case PHOTOMETRIC_PALETTE:
    return "colormap ";
  case PHOTOMETRIC_MASK:
    return "masked ";
  case PHOTOMETRIC_SEPARATED:
    return "color-separated ";
  case PHOTOMETRIC_YCBCR:
    return "YCBCR ";
  case PHOTOMETRIC_CIELAB:
    return "CIE L*a*b* ";
  default:
    return "";
  }
}

static char *compressionName(compression)
     int compression;
{
  switch (compression) {
  case COMPRESSION_NONE:
    return "standard ";
  case COMPRESSION_CCITTRLE:
    return "RLE ";
  case COMPRESSION_CCITTFAX3:
    return "G3FAX ";
  case COMPRESSION_CCITTFAX4:
    return "G4FAX ";
  case COMPRESSION_LZW:
    return "LZW ";
  case COMPRESSION_JPEG:
    return "JPEG ";
  case COMPRESSION_NEXT:
    return "NeXT ";
  case COMPRESSION_CCITTRLEW:
    return "RLEW ";
  case COMPRESSION_PACKBITS:
    return "Macintosh ";
  case COMPRESSION_THUNDERSCAN:
    return "Thunderscan ";
  default:
    return "";
  }
}

static char *planarConfigName(config)
     int config;
{
  switch (config) {
  case PLANARCONFIG_CONTIG:
    return "single-plane ";
  case PLANARCONFIG_SEPARATE:
    return "separate-plane ";
  default:
    return "";
  }
}

static void babble(name, info)
     char *name;
     struct tiff_info *info;
{
  switch (info->photometric) {
  case PHOTOMETRIC_MINISWHITE:
  case PHOTOMETRIC_MINISBLACK:
    printf("%s is a %dx%d %s%s%sTIFF image",
	   name, info->width, info->height,
	   planarConfigName(info->planarconfiguration),
	   photometricName(info),
	   compressionName(info->compression));
    break;
  default:
    printf("%s is a %dx%d %d-bit %s%s%sTIFF image",
	   name, info->width, info->height,
	   (info->bitspersample * info->samplesperpixel),
	   planarConfigName(info->planarconfiguration),
	   photometricName(info),
	   compressionName(info->compression));
  }
  if (info->title)
    printf("Titled \"%s\"");
  printf("\n");
}

int tiffIdent(fullname, name)
{
  TIFF *tiff;
  struct tiff_info info;

  tiff = is_tiff(fullname, name, &info);
  babble(name, info);
  if (tiff == NULL)
    return(0);
  if (tiff == (TIFF *)-1) /* is TIFF, but can't open it */
    return(1);
  TIFFClose(tiff);

  babble(fullname, name, info);
  return(1);
}

Image *tiffLoad(fullname, name, verbose)
     char *fullname;
     char *name;
     int verbose;
{
  TIFF *tiff;
  struct tiff_info info;
  Image *image;
  byte *row, *dest_line;
  int dest_line_len;
  register int pixlen;
  register byte *dest;
  register int x, y;

  tiff = is_tiff(fullname, name, &info);
  if ((tiff == NULL) || (tiff == (TIFF *)-1))
    return(NULL);
  if (verbose)
    babble(name, &info);

  row = lmalloc(info.bytesperrow);

  /* decide which type of image to allocate based on photometric
   * information and set up the colormap as necessary
   */
  switch (info.photometric) {
  case PHOTOMETRIC_MINISWHITE: /* bitmap and greyscale image types */
  case PHOTOMETRIC_MINISBLACK:
    /* monochrome image
     */
    if (info.bitspersample == 1) {
      dest_line_len = (info.width / 8) + (info.width % 8 ? 1 : 0);
      if (dest_line_len > info.bytesperrow) /* just in case */
	dest_line_len = info.bytesperrow;

      image= newBitImage(info.width, info.height);

      /* default bit image colormap matches PHOTOMETRIC_MINISWHITE, so we
       * need to change it if it's the other type.
       */
      if (info.photometric == PHOTOMETRIC_MINISBLACK) {
	image->rgb.red[0]= image->rgb.green[0]= image->rgb.blue[0]= 0;
	image->rgb.red[1]= image->rgb.green[1]= image->rgb.blue[1]= 65535;
      }

      /* read the image data and set the pixels
       */
      dest = image->data;
      if (info.bitspersample == 1) {
	for (y = 0; y < info.height; y++) {
	  if (TIFFReadScanline(tiff, row, y, 0) < 0) {
	    fprintf(stderr, "%s: Short read in image data!\n", fullname);
	    break;
	  }

	  /* isn't it nice when it's in the same data format we use?
	   */
	  bcopy(row, dest, dest_line_len);
	  dest += dest_line_len;
	}
      }
      break;
    }
    else {
      /* need to build the scale for greyscale images
       */
      image = newRGBImage(info.width, info.height, info.bitspersample);
      for (x = 0; x < image->rgb.size; x++) {
	int value = (65535 * x) / (unsigned int)(image->rgb.size);
	if (info.photometric == PHOTOMETRIC_MINISWHITE)
	  value = 65535 - value;
	image->rgb.red[x]= image->rgb.green[x]= image->rgb.blue[x]= value;
      }
      image->rgb.used = image->rgb.size;
      goto read_pallette_data; /* ugly but expedient */
    }
    /* NOTREACHED */

  case PHOTOMETRIC_PALETTE:
    /* this is a close match with the IRGB-style Image.
     */
    image = newRGBImage(info.width, info.height, info.bitspersample);
    {
      unsigned short *red, *green, *blue;
 
      if (!TIFFGetField(tiff, TIFFTAG_COLORMAP, &red, &green, &blue)) {
	fprintf(stderr, "%s: Image has no colormap!\n", fullname);
	freeImage(image);
	image = NULL;
	break;
      }

      /* fill in our colormap with this data
       */
      switch (info.type) {
      case TIFF_BIGENDIAN:
	for (x = 0; x < image->rgb.size; x++) {
	  image->rgb.red[x] = memToVal(&red[x], 2);
	  image->rgb.green[x] = memToVal(&green[x], 2);
	  image->rgb.blue[x] = memToVal(&blue[x], 2);
	}
	break;
      case TIFF_LITTLEENDIAN:
	for (x = 0; x < image->rgb.size; x++) {
	  image->rgb.red[x] = memToValLSB(&red[x], 2);
	  image->rgb.green[x] = memToValLSB(&green[x], 2);
	  image->rgb.blue[x] = memToValLSB(&blue[x], 2);
	}
	break;
      }
      image->rgb.used = image->rgb.size; /* all filled up */
    }

  read_pallette_data:
    /* start reading image data
     */
    dest_line_len = info.width * image->pixlen;
    if (dest_line_len > info.bytesperrow) /* just in case */
      dest_line_len = info.bytesperrow;
    pixlen = image->pixlen;
    dest_line = image->data;

    switch (info.type) {
    case TIFF_LITTLEENDIAN:
      if (info.bitspersample > 8) {

	/* bummer, need to byte-swap
	 */
	for (y = 0; y < info.height; y++) {
	  if (TIFFReadScanline(tiff, row, y, 0) < 0) {
	    fprintf(stderr, "%s: Short read in image data!\n", fullname);
	    break;
	  }
	  dest = dest_line;
	  for (x = 0; x < info.width; x++) {
	    valToMem(memToValLSB(dest, image->pixlen), dest, pixlen);
	    dest += pixlen;
	  }
	  dest_line += dest_line_len;
	}
      }
      /* if the sample fits in 1 byte we don't need to byte swap so
       * we fall through into the big-endian reader which is much faster.
       */
      /* FALLTHRU */
    case TIFF_BIGENDIAN:
      /* bigendian is a nice match for our internal data format
       */
      for (y = 0; y < info.height; y++) {
	if (TIFFReadScanline(tiff, row, y, 0) < 0) {
	  fprintf(stderr, "%s: Short read in image data!\n", fullname);
	  break;
	}
	bcopy(row, dest_line, dest_line_len);
	dest_line += dest_line_len;
      }
      break;
    }
    break;

  case PHOTOMETRIC_RGB:
    /* this is a close match with the ITRUE-style Image.
     */
    if (info.samplesperpixel != 3) {
      fprintf(stderr,
	      "%s: Can't handle TIFF RGB images with %d samples per pixel, sorry\n",
	      info.samplesperpixel);
      image = NULL;
      break;
    }
    image = newTrueImage(info.width, info.height);
    dest_line_len = info.width * image->pixlen;
    if (dest_line_len > info.bytesperrow) /* just in case */
      dest_line_len = info.bytesperrow;
    dest_line = image->data;

    /* quick-and-dirty copy if it's just what we're looking for
     */
    switch (info.planarconfiguration) {
    case PLANARCONFIG_CONTIG:
      if (info.bitspersample == 8) {
	for (y = 0; y < info.height; y++) {
	  if (TIFFReadScanline(tiff, row, y, 0) < 0) {
	    fprintf(stderr, "%s: Short read in image data!\n", fullname);
	    break;
	  }
	  bcopy(row, dest_line, dest_line_len);
	  dest_line += dest_line_len;
	}
      }
      else {
    case PLANARCONFIG_SEPARATE:
	fprintf(stderr, "%s: %s is an unsupported planar configuration.\n",
		fullname, planarConfigName(info.planarconfiguration));
	freeImage(image);
	image = NULL;
      }
    }
    break;

  case PHOTOMETRIC_MASK:
  case PHOTOMETRIC_SEPARATED:
  case PHOTOMETRIC_YCBCR:
  case PHOTOMETRIC_CIELAB:
  default:
    fprintf(stderr, "%s: %s is an unsupported TIFF photometric style, sorry.\n",
	    fullname, photometricName(&info));
    image = NULL;
  }

  if (image) {
    if (info.title)
      image->title = info.title;
    else
      image->title = dupString(name);
  }

  TIFFClose(tiff);
  lfree(row);
  return(image);
}

/* this is not what I'd call a well-designed TIFF dumping function but it
 * does do the job.
 */
void tiffDump(image, options, file, verbose)
     Image *image;
     char *options;
     char *file;
     int verbose;
{
  TIFF *out;
  char *name, *value;
  int compression = COMPRESSION_LZW;
  int y;
  int srclinelen;
  byte *srcptr;

  out = TIFFOpen(file, "w");
  TIFFSetField(out, TIFFTAG_IMAGEWIDTH, (unsigned long) image->width);
  TIFFSetField(out, TIFFTAG_IMAGELENGTH, (unsigned long) image->height);
  TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, 1);

  /* this sets the TIFF resolution, necessary for some packages.  since
   * we have no concept of resolution (most input files don't retain that
   * information) we set it for 1:1 pixel resolution.
   */
  TIFFSetField(out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
  TIFFSetField(out, TIFFTAG_XRESOLUTION, (float)1.0);
  TIFFSetField(out, TIFFTAG_YRESOLUTION, (float)1.0);

  /* process image options
   */
  while (getNextTypeOption(&options, &name, &value) > 0) {
    if (!strncmp("compression", name, strlen(name))) {
      if (!strncmp("none", value, strlen(value)))
	compression = COMPRESSION_NONE;
      else if (!strncmp("rle", value, strlen(value)))
	compression = COMPRESSION_CCITTRLE;
      else if (!strncmp("g3fax", value, strlen(value)))
	compression = COMPRESSION_CCITTFAX3;
      else if (!strncmp("g4fax", value, strlen(value)))
	compression = COMPRESSION_CCITTFAX4;
      else if (!strncmp("lzw", value, strlen(value)))
	compression = COMPRESSION_LZW; /* default */
      else if (!strncmp("jpeg", value, strlen(value)))
	compression = COMPRESSION_JPEG;
      else if (!strncmp("next", value, strlen(value)))
	compression = COMPRESSION_NEXT;
      else if (!strncmp("rlew", value, strlen(value)))
	compression = COMPRESSION_CCITTRLEW;
      else if (!strncmp("packbits", value, strlen(value)) ||
	       !strncmp("mac", value, strlen(value)))
	compression = COMPRESSION_PACKBITS;
      else if (!strncmp("thunderscan", value, strlen(value)))
	compression = COMPRESSION_THUNDERSCAN;
      else
	fprintf(stderr, "tiffDump: Unknown compression type '%s'.\n",
		value);
    }
    else
      fprintf(stderr, "tiffDump: Unknown option '%s'\n", name);
  }
	
  TIFFSetField(out, TIFFTAG_COMPRESSION, compression);

  switch (image->type) {
  case IBITMAP:
    /* should use pixel intensity but this will usually work */
    if (image->rgb.red[0] > image->rgb.red[1]) {
      if (verbose) {
	printf("Dumping black-on-white ");
	if (compression != COMPRESSION_NONE)
	  printf("%s", compressionName(compression));
	printf("TIFF image to %s.\n", file);
      }
      TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
    }
    else {
      if (verbose) {
	printf("Dumping white-on-black ");
	if (compression != COMPRESSION_NONE)
	  printf("%s", compressionName(compression));
	printf("TIFF image to %s.\n", file);
      }
      TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    }
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 1);

    srclinelen = (image->width / 8) + (image->width % 8 ? 1 : 0);
    srcptr = image->data;
    for (y = 0; y < image->height; y++) {
      TIFFWriteScanline(out, srcptr, y, 0);
      srcptr += srclinelen;
    }
    break;

  case IRGB:
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8 * image->pixlen);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);

    if (verbose) {
      printf("Dumping colormap ");
      if (compression != COMPRESSION_NONE)
	printf("%s", compressionName(compression));
      printf("TIFF image to %s.\n", file);
    }

    /* save the colormap
     */
    {
      unsigned short *red, *green, *blue;

      red = (unsigned short *)lmalloc(image->rgb.size * sizeof(unsigned short));
      green = (unsigned short *)lmalloc(image->rgb.size * sizeof(unsigned short));
      blue = (unsigned short *)lmalloc(image->rgb.size * sizeof(unsigned short));

      for (y = 0; y < image->rgb.used; y++) {
	valToMem(image->rgb.red[y], &red[y], 2);
	valToMem(image->rgb.green[y], &green[y], 2);
	valToMem(image->rgb.blue[y], &blue[y], 2);
      }
      while (y < image->rgb.size) {
	valToMem(0, &red[y], 2);
	valToMem(0, &green[y], 2);
	valToMem(0, &blue[y], 2);
	y++;
      }
      TIFFSetField(out, TIFFTAG_COLORMAP, red, green, blue);
      lfree((byte *)red);
      lfree((byte *)green);
      lfree((byte *)blue);
    }
    srclinelen = image->width * image->pixlen;
    srcptr = image->data;
    for (y = 0; y < image->height; y++) {
      TIFFWriteScanline(out, srcptr, y, 0);
      srcptr += srclinelen;
    }
    break;

  case ITRUE:
    if (verbose) {
      printf("Dumping RGB ");
      if (compression != COMPRESSION_NONE)
	printf("%s", compressionName(compression));
      printf("TIFF image to %s.\n", file);
    }
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    srclinelen = image->width * image->pixlen;
    srcptr = image->data;
    for (y = 0; y < image->height; y++) {
      TIFFWriteScanline(out, srcptr, y, 0);
      srcptr += srclinelen;
    }
    break;
  }
  TIFFClose(out);
}

#else /* !HAS_TIFF */
static int unused;
#endif /* !HAS_TIFF */
