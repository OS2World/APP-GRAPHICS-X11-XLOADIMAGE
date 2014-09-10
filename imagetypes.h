/* imagetypes.h:
 *
 * supported image types and the imagetypes array declaration.  when you
 * add a new image type, only the makefile and this header need to be
 * changed.
 *
 * jim frost 10.15.89
 */

Image *niffLoad();
Image *facesLoad();
Image *pbmLoad();
Image *sunRasterLoad();
Image *gifLoad();
Image *rleLoad();
Image *xwdLoad();
Image *vffLoad();
Image *xbitmapLoad();
Image *xpixmapLoad();
Image *fbmLoad();
Image *pcxLoad();
Image *imgLoad();
Image *macLoad();
Image *cmuwmLoad();
Image *mcidasLoad();
#if 0
Image *pdsLoad();
#else
Image *vicarLoad();
#endif
#ifdef HAS_JPEG
Image *jpegLoad();
#endif
#ifdef HAS_TIFF
Image *tiffLoad();
#endif

int niffIdent();
int facesIdent();
int pbmIdent();
int sunRasterIdent();
int gifIdent();
int rleIdent();
int xwdIdent();
int vffIdent();
int xbitmapIdent();
int xpixmapIdent();
int fbmIdent();
int pcxIdent();
int imgIdent();
int macIdent();
int cmuwmIdent();
int mcidasIdent();
#if 0
int pdsIdent();
#else
int vicarIdent();
#endif
#ifdef HAS_JPEG
int jpegIdent();
#endif
#ifdef HAS_TIFF
int tiffIdent();
#endif

void niffDump();
#ifdef HAS_JPEG
void jpegDump();
#endif
#ifdef HAS_TIFF
void tiffDump();
#endif
void pbmDump();

/* some of these are order-dependent
 */

struct {
  int    (*identifier)(); /* print out image info if this kind of image */
  Image *(*loader)();     /* load image if this kind of image */
  void   (*dumper)();     /* dump image of this kind */
  char  *type;            /* image type name */
  char  *name;            /* name of this image format */
} ImageTypes[] = {
  niffIdent,      niffLoad,      niffDump,    "niff",      "Native Image File Format (NIFF)",
  sunRasterIdent, sunRasterLoad, NULL,        "sunraster", "Sun Rasterfile",
  gifIdent,       gifLoad,       NULL,        "gif",       "GIF Image",
#ifdef HAS_JPEG
  jpegIdent,      jpegLoad,      jpegDump,    "jpeg",      "JFIF-style JPEG Image",
#endif
#ifdef HAS_TIFF
  tiffIdent,      tiffLoad,      tiffDump,    "tiff",      "TIFF image",
#endif
  fbmIdent,       fbmLoad,       NULL,        "fbm",       "FBM Image",
  cmuwmIdent,     cmuwmLoad,     NULL,        "cmuraster", "CMU WM Raster",
  pbmIdent,       pbmLoad,       pbmDump,     "pbm",       "Portable Bit Map (PBM, PGM, PPM)",
  facesIdent,     facesLoad,     NULL,        "faces",     "Faces Project",
  rleIdent,       rleLoad,       NULL,        "rle",       "Utah RLE Image",
  xwdIdent,       xwdLoad,       NULL,        "xwd",       "X Window Dump",
  vffIdent,       vffLoad,       NULL,        "vff",       "Sun Visualization File Format",
  mcidasIdent,    mcidasLoad,    NULL,        "mcidas",    "McIDAS areafile",
#if 0
  pdsIdent,       pdsLoad,       NULL,        "pds",       "PDS/VICAR Image",
#else
  vicarIdent,     vicarLoad,     NULL,        "vicar",     "VICAR Image",
#endif
  pcxIdent,       pcxLoad,       NULL,        "pcx",       "PC Paintbrush Image",
  imgIdent,       imgLoad,       NULL,        "gem",       "GEM Bit Image",
  macIdent,       macLoad,       NULL,        "macpaint",  "MacPaint Image",
  xpixmapIdent,   xpixmapLoad,   NULL,        "xpm",       "X Pixmap",
  xbitmapIdent,   xbitmapLoad,   NULL,        "xbm",       "X Bitmap",
  NULL,           NULL,          NULL,         NULL,        NULL
};
