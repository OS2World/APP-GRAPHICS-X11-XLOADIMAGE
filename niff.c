/* niff.c:
 *
 * xloadimage native image format.  This is used for loading and dumping
 * xloadimage images.
 *
 * jim frost 10.29.91
 *
 * this is in the public domain.
 */

#include "image.h"
#include "niff.h"

static void babble(name, header, title)
     struct niff_header *header;
     char *title;
{
  printf("%s is a %dx%d ", name,
	 memToVal(header->width, 4),
	 memToVal(header->height, 4),
	 memToVal(header->depth, 4));
  if (memToVal(header->version, 4) != NIFF_VERSION)
    printf("version %d ", memToVal(header->version, 4));
  printf("NIFF ");
  switch (header->type) {
  case NIFF_BITMAP:
    printf("bitmap image");
    break;
  case NIFF_RGB:
    printf("%d-bit RGB image with %d colors",
	   memToVal(header->depth, 4), memToVal(header->cmap_size, 4));
    break;
  case NIFF_TRUE:
    printf("%d-bit true color image", memToVal(header->depth, 4));
    break;
  default:
    printf("image with an unknown type");
  }
  if (title && title[0])
    printf(" entitled \"%s\"", title);
  printf("\n");
}

static int readHeader(zf, header, rtitle)
     ZFILE *zf;
     struct niff_header *header;
     char **rtitle;
{ unsigned int title_len;
  char *title = NULL;

  /* read in header
   */
  if (zread(zf, (byte *)header, sizeof(struct niff_header)) !=
      sizeof(struct niff_header)) {
    zclose(zf);
    return(0);
  }

  /* check magic number to see if this is a NIFF image
   */
  if (memToVal(header->magic, 4) != NIFF_MAGIC)
    return(0);

  /* read in title if there is one
   */
  title_len= memToVal(header->title_len, 4);
  if (title_len != 0) {
    title= (char *)lmalloc(title_len + 1);
    if (zread(zf, (byte *)title, title_len) != title_len) {
      lfree((byte *)title);
      return(0);
    }
    title[title_len]= '\0';
  }
  if (rtitle)
    *rtitle= title;
  else
    lfree((byte *)title);
  return(1);
}

int niffIdent(fullname, name)
     char *fullname, *name;
{ ZFILE *zf;
  struct niff_header header;
  char *title;

  if (! (zf= zopen(fullname)))
    return(0);

  if (!readHeader(zf, &header, &title))
    return(0);
  zclose(zf);
  babble(name, &header, title);
  lfree((byte *)title);
  return(1);
}

static int readColormap(zf, image)
     ZFILE *zf;
     Image *image;
{ unsigned int a;
  struct niff_cmap cmap;

  for (a= 0; a < image->rgb.used; a++) {
    if (zread(zf, (byte *)&cmap, sizeof(struct niff_cmap)) != sizeof(struct niff_cmap)) {
      fprintf(stderr, "niffLoad: short read in colormap!\n");
      return(0);
    }
    image->rgb.red[a]= memToVal(cmap.red, 2);
    image->rgb.green[a]= memToVal(cmap.green, 2);
    image->rgb.blue[a]= memToVal(cmap.blue, 2);
  }
  return(1);
}

Image *niffLoad(fullname, name, verbose)
     char *fullname, *name;
     unsigned int verbose;
{ ZFILE *zf;
  struct niff_header header;
  char *title;
  unsigned int width, height, depth;
  Image *image;
  unsigned int data_size;

  if (! (zf= zopen(fullname)))
    return(NULL);

  if (!readHeader(zf, &header, &title)) {
    zclose(zf);
    return(NULL);
  }
  if (verbose)
    babble(name, &header, title);
  znocache(zf);

  width= memToVal(header.width, 4);
  height= memToVal(header.height, 4);
  depth= memToVal(header.depth, 4);
  switch (header.type) {
  case NIFF_BITMAP:
    image= newBitImage(width, height);
    if (memToVal(header.cmap_size, 4) != 2)
      fprintf(stderr, "niffLoad: bitmap image has wrong number of colormap entries!\n");
    if (!readColormap(zf, image)) {
      freeImage(image);
      zclose(zf);
      return(NULL);
    }
    data_size= ((image->width / 8) + (image->width % 8 ? 1 : 0)) * image->height;
    break;
  case NIFF_RGB:
    image= newRGBImage(width, height, depth);
    image->rgb.used= memToVal(header.cmap_size, 4);
    if (image->rgb.used > image->rgb.size) {
      fprintf(stderr, "niffLoad: too many colormap entries!\n");
      image->rgb.used= image->rgb.size;
    }
    if (!readColormap(zf, image)) {
      freeImage(image);
      zclose(zf);
      return(NULL);
    }
    data_size= image->width * image->height * image->pixlen;
    break;
  case NIFF_TRUE:
    image= newTrueImage(width, height);
    data_size= image->width * image->height * 3;
  }
  image->title= title;

  /* read in image data
   */

  if (zread(zf, image->data, data_size) != data_size)
    fprintf(stderr, "Short read on image data\n");
  zclose(zf);
  return(image);
}

/* this takes an Image and dumps it into a file in NIFF format.  returns
 * zero if successful.
 */
void niffDump(image, options, filename, verbose)
     Image *image;
     char *options; /* ignored */
     char *filename;
     int verbose;
{ FILE *f;
  unsigned int a;
  struct niff_header header;
  struct niff_cmap cmap;
  unsigned int data_size;

  if (verbose)
    printf("Dumping NIFF image to %s.\n", filename);

  if (!(f= fopen(filename, "w"))) {
    perror(filename);
    return;
  }

  valToMem(NIFF_MAGIC, header.magic, 4);
  valToMem(NIFF_VERSION, header.version, 4);
  valToMem(image->width, header.width, 4);
  valToMem(image->height, header.height, 4);
  valToMem(image->depth, header.depth, 4);
  switch (image->type) {
  case IBITMAP:
    header.type= NIFF_BITMAP;
    valToMem(image->rgb.used, header.cmap_size, 4);
    data_size= ((image->width / 8) + (image->width % 8 ? 1 : 0)) * image->height;
    break;
  case IRGB:
    header.type= NIFF_RGB;
    valToMem(image->rgb.used, header.cmap_size, 4);
    data_size= image->width * image->height * image->pixlen;
    break;
  case ITRUE:
    header.type= NIFF_TRUE;
    valToMem(0, header.cmap_size, 4);
    data_size= image->width * image->height * 3;
    break;
  }
  valToMem((image->title ? strlen(image->title) : 0), header.title_len, 4);

  /* write header
   */
  if (fwrite(&header, sizeof(struct niff_header), 1, f) != 1) {
    perror(filename);
    unlink(filename);
    return;
  }

  /* write title if there is one
   */
  if (image->title && (strlen(image->title) > 0)) {
    if (fwrite(image->title, strlen(image->title), 1, f) != 1) {
      perror(filename);
      unlink(filename);
      return;
    }
  }

  /* write cmap if we need one
   */
  switch (image->type) {
  case IBITMAP:
  case IRGB:
    for (a= 0; a < image->rgb.used; a++) {
      valToMem(image->rgb.red[a], cmap.red, 2);
      valToMem(image->rgb.green[a], cmap.green, 2);
      valToMem(image->rgb.blue[a], cmap.blue, 2);
      if (fwrite(&cmap, sizeof(struct niff_cmap), 1, f) != 1) {
	perror(filename);
	unlink(filename);
	return;
      }
    }
  }

  /* write image data
   */

  if (fwrite(image->data, data_size, 1, f) != 1) {
    perror(filename);
    unlink(filename);
    return;
  }
  return;
}
