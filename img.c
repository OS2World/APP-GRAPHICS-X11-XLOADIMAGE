/* GEM-Images
 *
 * dieter fiebelkorn GEM image type
 *
 * dieter fiebelkorn 19.12.90
 *
 * jimf 10.29.93 - lots of reworking to handle different byte-orders and
 *                 32-bit integers.  no color images to test it with...
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "image.h"

extern int errno;

#define TRUE  1
#define FALSE 0

#define MAX_PLANES 8
#define ALL_PLANES 8


#define LEAVE {\
		 errorInLoad = errno;\
		 printf("  Short read within image data\n");\
	         return;\
	      }

#define LEAVE_ON_ERROR(func) if ((func) <= 0) LEAVE



static int           vdiColorOrder = 0;
static char         *depthError = "Unsupported depth (%d) in IMG-file.\n";
static unsigned int  out_depthError = 0;
int                  errorInLoad;


typedef struct IMGHEADER
{
  byte im_version[2];
  byte im_headlength[2];
  byte im_nplanes[2];
  byte im_patlen[2];
  byte im_pixwidth[2];
  byte im_pixheight[2];
  byte im_scanwidth[2];
  byte im_nlines[2];
} IMGHEADER;


typedef struct ximg_header
{
  byte im_version[2];
  byte im_headlength[2];
  byte im_nplanes[2];
  byte im_patlen[2];
  byte im_pixwidth[2];
  byte im_pixheight[2];
  byte im_scanwidth[2];
  byte im_nlines[2];
  char x_id [4];         /* must contain "XIMG" */
  byte color_model[2];   /* 0 = RGB,1 = CYM,2 = HLS, etc. */
} XIMG_HEADER;

void (*transf)    ();

static int vdi2pli();
static int pli2vdi();

static void   transf0    ();
static void   transf1    ();
static void   transf2    ();
static void   transf3    ();
static void   xread_img  ();
static void   xread_line ();
static int    ident_img  ();
static Image *load_img   ();
static int    ident_ximg ();
static Image *load_ximg  ();

IMGHEADER               header;
XIMG_HEADER             xheader;
unsigned int            ScanByteBreite, max_planes,
                        pattern_len, plane, x, y, ByteNr, height;
unsigned char           *ptr, Muster[ALL_PLANES][BUFSIZ], tmp[BUFSIZ];
static unsigned char    *bitplane[ALL_PLANES], *lastbitplane;



static void transf0(bitimage, plane, value, planes)
     unsigned char *bitimage;
     unsigned int   plane;
     unsigned char  value;
     unsigned int   planes;
{
  bitimage[x] = value;
  x++;
}



static void transf1(bitimage, plane, value, planes)
     unsigned char *bitimage;
     unsigned int   plane;
     unsigned char  value;
     unsigned int   planes;
{
  unsigned char  bit;
  unsigned long  off;

  bit= 0x80;
  off= 0;
  do
  {
    if (bit & value)
      bitimage[off + 8*x] |= (unsigned char)0x01 << plane;
    bit >>= 1;
    off++;
  }
  while (bit > 0);
  x++;
}



static void transf2(bitimage, plane, value, planes)
     unsigned char *bitimage;
     unsigned int   plane;
     unsigned char  value;
     unsigned int   planes;
{
  int    i;
  
  if (bitimage != lastbitplane)
  {
    lastbitplane = bitimage;
    for (i = 0; i < planes; i++)
      bitplane[i] += ScanByteBreite + (ScanByteBreite % 2);
  }
  bitplane[plane][x] = value;
  x++;
}

static void transf3(bitimage, plane, value, planes)
     unsigned char *bitimage;
     unsigned int   plane;
     unsigned char  value;
     unsigned int   planes;
{
  unsigned char  bit;
  unsigned long  off;

  bit= 0x80;
  off= 0;
  do
  {
    if (bit & value)
    {
      if (plane <  8)
        bitimage[off + 24*x+2] |= (unsigned char)0x01 << plane;
      if (plane >=  8 && plane < 16)
        bitimage[off + 24*x+1] |= (unsigned char)0x01 << (plane-8);
      if (plane >= 16 && plane < 24)
        bitimage[off + 24*x  ] |= (unsigned char)0x01 << (plane-16);
    }
    bit >>= 1;
    off+=3;
  }
  while (bit > 0);
  x++;
}
  
static void xread_line (file, planes)
ZFILE   *file;
int     planes;
{
  ByteNr= 0;
  for (plane= 0; plane < planes; plane++)
  {
    do
      xread_img (file, TRUE, planes);
    while (ByteNr < ScanByteBreite);
    ByteNr= 0;
    x= 0;
  }
}



static void xread_img (file, linie, planes)
ZFILE           *file;
unsigned int    linie;
int             planes;
{
  unsigned int    wert, i, k;
  unsigned char   status;

  wert = zgetc(file);
  switch (wert)
  {
    case EOF:
      LEAVE;
    case 0x00:
      wert = zgetc(file);
      switch (wert)
      {
        case EOF:
	  LEAVE;
        case 0x00: /* Vertical Run */
          LEAVE_ON_ERROR(zread(file, tmp, 2L));
          if (tmp[0] == 0xFF && !linie)
          {
            wert = tmp[1];
            xread_line (file, planes);
          }
          else
          {
            errorInLoad = 1;
            printf("  Wrong (X)IMG format!");
            return;
          }
          if (y + wert > height)
            wert = height - y;
          if (planes == 1)
          {
            for (k= 0; k < wert; k++)
            {
              for (i= 0; i < ScanByteBreite; i++)
                *ptr++ = Muster[0][i];
              if (ScanByteBreite % 2 == 1)
                *ptr++ = 0x00;
            }
            ByteNr = ScanByteBreite;
            if (ByteNr % 2 == 1)
              ByteNr++;
            x = 0;
            y += (wert - 1);
          }
          else
          {
            for (k= 0; k < wert; k++)
            {
              int plane;
              for (plane= 0; plane < planes; plane++)
              {
                x= 0;
                for (ByteNr= 0; ByteNr < ScanByteBreite; ByteNr++)
                  (*transf)(ptr, plane, Muster[plane][ByteNr], planes);
              }
              ptr += max_planes * (ScanByteBreite + (ScanByteBreite % 2));
              y++;
            }
            x = 0;
            ptr -= max_planes * (ScanByteBreite + (ScanByteBreite % 2));
            y--;
          }
          break;
        default  : /* Pattern Run */
          if (!linie)
          {
            LEAVE_ON_ERROR(zread(file, tmp, (long)pattern_len));
            if (planes == 1)
            {
              for (k= 0; k < wert && ByteNr < ScanByteBreite; k++)
              {
                for (i= 0; i < pattern_len; i++)
                  *ptr++ = tmp[i];
                ByteNr += pattern_len;
              }
            }
            else
            {
              for (k= 0; k < wert && ByteNr < ScanByteBreite; k++)
              {
                for (i= 0; i < pattern_len; i++)
                  (*transf)(ptr, plane, tmp[i], planes);
                ByteNr += pattern_len;
              }
            }
          }
          else
          {
	    LEAVE_ON_ERROR(zread(file, tmp, (long)pattern_len));

            for (k= 0; k < wert && ByteNr < ScanByteBreite; k++)
              for (i= 0; i < pattern_len; i++)
                Muster[plane][ByteNr++] = tmp[i];
          }
      }
      break;
    case 0x80:
      wert = zgetc(file);
      if (wert == EOF)
	LEAVE;

      i = wert;
      if (ByteNr + i > ScanByteBreite)
        i = ScanByteBreite - ByteNr;
      wert -= i;
      if (!linie)
      {
        if (planes == 1)
        {
          zread(file, ptr, (long)i);
          ptr += i;
          ByteNr += i;
          if (wert)
	    LEAVE_ON_ERROR(zread(file, tmp, (long)wert));

        }
        else
        {
          zread(file, tmp, (long)i);
          for (k = 0; k < i; k++)
          {
            (*transf)(ptr, plane, tmp[k], planes);
            ByteNr++;
          }
          if (wert)
            LEAVE_ON_ERROR(zread(file, tmp, (long)wert));
        }
      }
      else
      {
        zread(file, &Muster[plane][ByteNr], (long)i);
        ByteNr += i;
        if (wert)
          LEAVE_ON_ERROR(zread(file, tmp, (long)wert));
      }
      break;
    default  :
      if (wert & 0x80)
        status = 0xff;
      else
        status = 0x00;
      wert &= 0x7f;
      if (!linie)
      {
        if (planes == 1)
          for (i= 0; i < wert && ByteNr < ScanByteBreite; i++)
          {
            *ptr++ = status;
            ByteNr++;
          }
        else
          for (i= 0; i < wert && ByteNr < ScanByteBreite; i++)
          {
            (*transf)(ptr, plane, status, planes);
            ByteNr++;
          }
      }
      else
        for (i= 0; i < wert && ByteNr < ScanByteBreite; i++)
          Muster[plane][ByteNr++] = status;
  }
}

static int ident_img(name)
     char *name;
{
  ZFILE   *file;
  long    size = 0;
  int     w, h, nplanes;
  char    extention[5];

#if 0
  strncpy (extention, name+strlen(name)-4, 4);
  extention[4] = '\0';
  if (strcmp(extention, ".IMG") != 0 && strcmp(extention, ".img") != 0)
    return (0);
#endif

  if ((file= zopen(name)) == NULL)
  {
    printf("  Error reading IMG!");
    return(0);
  }
  size = zread(file, (byte *)&header, sizeof(header));
  zclose(file);

  if (size < sizeof(header) || memToVal(header.im_version, 2) > 2)
    return (0);

  w = memToVal(header.im_scanwidth, 2);
  h = memToVal(header.im_nlines, 2);
  nplanes = memToVal(header.im_nplanes, 2);

  if ((nplanes > 4 && nplanes != 24) || nplanes == 0)
  {
    if (out_depthError == 0)
      printf(depthError, header.im_nplanes);
    return(0);
  }

  if (nplanes == 1)
    printf("%s is a %dx%d monochrome IMG-file\n",name , w, h);
  else
    if (nplanes != 24)
      printf("%s is a %dx%d IMG-file with %ld colors\n",name , w, h,
	     (1 << nplanes));
    else
      printf("%s is a %dx%d %d-bit IMG-file\n",name , w, h, nplanes);
  return (1);
}

static Image *load_img(name)
     char *name;
{
  ZFILE           *file;
  Image           *image;
  unsigned long    w, h, nplanes, headlength, scanwidth;
  int              colors;
  long             i, dummy;

  if ((file = zopen(name)) == NULL)
  {
    printf("  Error reading IMG!");
    return(NULL);
  }
  zread(file, (byte *)&header, sizeof(header));

  headlength = memToVal(header.im_headlength, 2);
  for (i=0; i<(headlength-(int)(sizeof(header)/2)); i++)
    zread(file, (byte *)&dummy, 2L);

  w = memToVal(header.im_scanwidth, 2);
  w = 16L * (w / 16L + (w % 16L ? 1 : 0));
  h = memToVal(header.im_nlines, 2);
  pattern_len = memToVal(header.im_patlen, 2);
  max_planes = 8;
  nplanes = memToVal(header.im_nplanes, 2);
  scanwidth = memToVal(header.im_scanwidth, 2);

  if (nplanes == 1)
  {
    transf = transf0;
    image= newBitImage(w, h);
    if (!image)
    {
      zclose(file);
      return(image);
    }
    plane = 0;

    ptr= image->data;
    x = y = ByteNr = 0;
    height = (int) h;
    ScanByteBreite = (scanwidth + 7) / 8;
    do
    {
      xread_img (file, FALSE, 1);
      if (ByteNr >= ScanByteBreite)
      {
        if (ByteNr % 2 == 1)
          *ptr++ = 0x00;
        ByteNr = 0;
        y++;
        x = 0;
      }
    }
    while (y < height && !errorInLoad);
  }
  if ((nplanes > 1 && nplanes <= 8) || nplanes == 24)
  {
    if (nplanes <= 8)
    {
      image= newRGBImage(w, h, nplanes);
      if (!image)
      {
        zclose(file);
        return(image);
      }
      transf = transf1;
      colors = 1 << nplanes;
    }
    else
    {
      image= newTrueImage(w, h);
      if (!image)
      {
        zclose(file);
        return(image);
      }
      transf = transf3;
      colors = 0;
      max_planes = 24;
    }
    if (nplanes >= 2 && nplanes <= 8)
    {
      image->rgb.red[ 0]= 0xFF00; image->rgb.green[ 0]= 0xFF00; image->rgb.blue[ 0]= 0xFF00;
      image->rgb.red[ 1]= 0xFF00; image->rgb.green[ 1]= 0x0000; image->rgb.blue[ 1]= 0x0000;
      image->rgb.red[ 2]= 0x0000; image->rgb.green[ 2]= 0xFF00; image->rgb.blue[ 2]= 0x0000;
      image->rgb.red[ 3]= 0x0000; image->rgb.green[ 3]= 0x0000; image->rgb.blue[ 3]= 0x0000;
    }
    if (nplanes >= 3 && nplanes <= 8)
    {
      image->rgb.red[ 3]= 0xFF00; image->rgb.green[ 3]= 0xFF00; image->rgb.blue[ 3]= 0x0000;
      image->rgb.red[ 4]= 0x0000; image->rgb.green[ 4]= 0x0000; image->rgb.blue[ 4]= 0xFF00;
      image->rgb.red[ 5]= 0xFF00; image->rgb.green[ 5]= 0x0000; image->rgb.blue[ 5]= 0xFF00;
      image->rgb.red[ 6]= 0x0000; image->rgb.green[ 6]= 0xFF00; image->rgb.blue[ 6]= 0xFF00;
      image->rgb.red[ 7]= 0x0000; image->rgb.green[ 7]= 0x0000; image->rgb.blue[ 7]= 0x0000;
    }
    if (nplanes >= 4 && nplanes <= 8)
    {
      image->rgb.red[ 7]= 0xDA00; image->rgb.green[ 7]= 0xDA00; image->rgb.blue[ 7]= 0xDA00;
      image->rgb.red[ 8]= 0x6D00; image->rgb.green[ 8]= 0x6D00; image->rgb.blue[ 8]= 0x6D00;
      image->rgb.red[ 9]= 0xB600; image->rgb.green[ 9]= 0x0000; image->rgb.blue[ 9]= 0x0000;
      image->rgb.red[10]= 0x0000; image->rgb.green[10]= 0xB600; image->rgb.blue[10]= 0x0000;
      image->rgb.red[11]= 0xB600; image->rgb.green[11]= 0xB600; image->rgb.blue[11]= 0x0000;
      image->rgb.red[12]= 0x0000; image->rgb.green[12]= 0x0000; image->rgb.blue[12]= 0xB600;
      image->rgb.red[13]= 0xB600; image->rgb.green[13]= 0x0000; image->rgb.blue[13]= 0xB600;
      image->rgb.red[14]= 0x0000; image->rgb.green[14]= 0xB600; image->rgb.blue[14]= 0xB600;
      image->rgb.red[15]= 0x0000; image->rgb.green[15]= 0x0000; image->rgb.blue[15]= 0x0000;
    }
    /* colors 17 to 255 undefined in this kind of IMG-File */
    
    image->rgb.used= colors;

    ptr= image->data;
    x = y = ByteNr = 0;
    height = (int) h;
    ScanByteBreite = (scanwidth + 7) / 8;
    do
    {
      for (plane= 0; plane < nplanes && !errorInLoad; plane++)
      {
        do
          xread_img (file, FALSE, nplanes);
        while (ByteNr < ScanByteBreite && !errorInLoad);
        ByteNr = 0;
        x = 0;
      }
      if (nplanes != 1)
        ptr += max_planes * (ScanByteBreite + (ScanByteBreite % 2));
      y++;
    }
    while (y < height && !errorInLoad);
  }

  if (errorInLoad > 0)
  {
    freeImage(image);
    image = NULL;
    zclose(file);
    return(image);
  }

  image->title= dupString(name);
  zclose(file);
  return(image);
}



static int ident_ximg(name)
     char *name;
{
  ZFILE  *file;
  long    size = 0;
  int     w, h, nplanes;
  char    extention[5];

  if ((file = zopen(name)) == NULL)
  {
    printf("  Error reading XIMG!");
    return(0);
  }
  size = zread(file, (byte *)&xheader, sizeof(xheader));
  zclose(file);

  if (size < sizeof(xheader))
    return (0);

  if (strncmp(xheader.x_id, "XIMG", 4)) /* || (xheader.color_model != 0 &&
                                           xheader.color_model != 'TC')) */
    return (0);

  w = memToVal(xheader.im_scanwidth, 2);
  h = memToVal(xheader.im_nlines, 2);
  nplanes = memToVal(xheader.im_nplanes, 2);
  if ((nplanes > MAX_PLANES && nplanes != 24) || nplanes == 0)
  {
    printf(depthError, xheader.im_nplanes);
    out_depthError = 1;
    return(0);
  }

  printf("%s\n  is a %dx%d ",name, w, h);
  if (nplanes != 24)
    printf("RGB XIMG-file with %ld colors\n", 1<<nplanes);
  else
    printf("24-bit XIMG-file\n");
  return (1);
}

static int vdi2pli(vdi, plimax)
     int vdi, plimax;
{
  extern unsigned char indexToCode[];
  extern int           work_ext[];
  static char          vdi2pli[] = {0, 15, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 11, 13 };
  
  if (vdi == 1)
    return(plimax-1);
  if (vdi >= 16)
    return(vdi);
  return(vdi2pli[vdi]);
}



static int pli2vdi(pli, plimax)
     int pli, plimax;
{
  static char pli2vdi[] = {0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1 };
  
  if (pli == plimax-1)
    return(1);
  if (pli >= 16)
    return(pli);
  return(pli2vdi[pli]);
}

static Image *load_ximg(name)
     char *name;
{
  void            transferRGBMap();
  ZFILE          *file;
  Image          *image;
  unsigned long   w, h, nplanes, scanwidth;
  int             i, color, colors;
  struct RGB_LIST
  {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
  } rgb_list;

  if ((file = zopen(name)) == NULL)
  {
    printf("  Error reading XIMG!");
    return(NULL);
  }
  zread(file, (byte *)&xheader, sizeof(xheader));
  
  w = scanwidth = memToVal(xheader.im_scanwidth, 2);
  w = 16L * (w / 16L + (w % 16L ? 1 : 0));
  h = memToVal(xheader.im_nlines, 2);
  nplanes = memToVal(xheader.im_nplanes, 2);
  pattern_len = memToVal(xheader.im_patlen, 2);
  max_planes = 8;

  if (nplanes == 1)
  {
    image= newBitImage(w, h);
    if (!image)
    {
      zclose(file);
      return(image);
    }
    transf = transf0;
  }
  if (nplanes > 1 && nplanes <= 8)
  {
    image= newRGBImage(w, h, nplanes);
    if (!image)
    {
      zclose(file);
      return(image);
    }
    transf = transf1;
  }

  colors = 1 << nplanes;

  if (nplanes == 24)
  {
    image= newTrueImage(w, h);
    if (!image)
    {
      zclose(file);
      return(image);
    }
    transf = transf0;
    nplanes = 1;
    colors = 0;
    max_planes =24;
  }

  for (color = 0; color < colors; color++)
  {
    zread (file, (byte *)&rgb_list, sizeof(rgb_list));
    if (vdiColorOrder)
    {
      image->rgb.red[vdi2pli(color, colors)]   = ((unsigned int) (255 * rgb_list.red   / 1000)) << 8;
      image->rgb.green[vdi2pli(color, colors)] = ((unsigned int) (255 * rgb_list.green / 1000)) << 8;
      image->rgb.blue[vdi2pli(color, colors)]  = ((unsigned int) (255 * rgb_list.blue  / 1000)) << 8;
    }
    else
    {
      image->rgb.red[color]   = ((unsigned int) (255 * rgb_list.red   / 1000)) << 8;
      image->rgb.green[color] = ((unsigned int) (255 * rgb_list.green / 1000)) << 8;
      image->rgb.blue[color]  = ((unsigned int) (255 * rgb_list.blue  / 1000)) << 8;
    }
  }
  if (colors != 0)
    image->rgb.used= (unsigned int) 1<<nplanes;
  else
    image->rgb.used= 0;
  
  ptr= image->data;
  x = y = ByteNr = 0;
  height = (int) h;
  
  if (colors != 0)
    ScanByteBreite = (scanwidth + 7) / 8;
  else
    ScanByteBreite = 24 * ((scanwidth + 7) / 8);
  do
  {
    for (plane= 0; plane < nplanes && !errorInLoad; plane++)
    {
      do
        xread_img (file, FALSE, nplanes);
      while (ByteNr < ScanByteBreite && !errorInLoad);
      if (nplanes == 1 && ByteNr % 2 == 1)
        *ptr++ = 0x00;
      ByteNr = 0;
      x = 0;
    }
    if (nplanes != 1)
      ptr += max_planes * (ScanByteBreite + (ScanByteBreite % 2));
    y++;
  }
  while (y < height && !errorInLoad);

  if (errorInLoad > 0)
  {
    freeImage(image);
    image = NULL;
    zclose(file);
    return(image);
  }
  image->title= dupString(name);

  zclose(file);
  return(image);
}

int imgIdent(name, name2, verbose)
     char *name;
     char *name2;
     int   verbose;
{
  int  identified;

  if ((identified = ident_ximg(name)) != 0)
    return(identified);
  identified = ident_img(name);

  return(identified);
}

Image *imgLoad(name, name2, verbose)
     char *name;
     char *name2;
     int   verbose;
{
  errorInLoad = 0;
  out_depthError = 0;
  if (ident_ximg(name) != 0)
    return(load_ximg(name));
  if (ident_img(name) != 0)
    return(load_img(name));

  return(NULL);
}
