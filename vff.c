/* vff.c
 * Sun Visualization File Format images
 *
 * -ksp  8 June 91
 */

/*
 * Copyright (c) 1991 Southwest Research Institute
 * All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  The name of Southwest Research
 * Institute may not be used to endorse or promote products derived
 * from this software without specific prior written permission.  The
 * author makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include "image.h"

#define HEAD_BUF_SIZE	2048
#define MAX_HEAD_LINES	100

static char *header[MAX_HEAD_LINES];
static int width, height, bands, cmsize;
static unsigned char rm[256], gm[256], bm[256];

static int readHeader();
static int parseHeader();
static int readImage();
static void freeheader();


Image *
vffLoad(fullname, name, verbose)
char *fullname, *name;
unsigned int verbose;
{

     ZFILE *zf;
     char buf[4];
     Image *image = NULL;

     if (!(zf = zopen(fullname)))
	  return(NULL);

     if (zread(zf, (unsigned char *)buf, 4) != 4) {
	  zclose(zf);
	  return(NULL);
     }

     if (strncmp(buf, "ncaa", 4) != 0) {
	  zclose(zf);
	  return(NULL);
     }

     if (readHeader(zf)) {
	  zclose(zf);
	  return(NULL);
     }

     if (parseHeader()) {
	  zclose(zf);
	  return(NULL);
     }

     if (bands == 1) {
	  image = newRGBImage(width, height, 8);
     } else if (bands == 2) {
	  printf("Only Sun VFF files with one or three+ bands are supported.\n");
	  zclose(zf);
	  return(NULL);
     } else if (bands >= 3) {
	  image = newTrueImage(width, height);
     }

     image->title = dupString(name);
     
     if (verbose) {
	  if (bands == 1) {
	       printf("%s is a %dx%d Sun VFF image with 1 band\n", name, width, height);
	  } else {
	       printf("%s is a %dx%d Sun VFF image with %d bands\n", name, width, height, bands);
	  }
     }

     if (readImage(zf, image)) {
	  freeImage(image);
	  zclose(zf);
	  return(NULL);
     }
     zclose(zf);
     
     return(image);
}

static int
readImage(zf, image)
ZFILE *zf;
Image *image;
{
     byte *buf;
     byte *op, *ip;
     int x, y, skip, linewidth, i;
     void memoryExhausted();

     if (bands == 1) {
	  if (cmsize == 0) {
	       /* build a grey ramp */
	       for (i = 0; i < 256; i++) {
		    *(image->rgb.red + i) = i * 256;
		    *(image->rgb.green + i) = i * 256;
		    *(image->rgb.blue + i) = i * 256;
	       }
	       image->rgb.used = 256;
	  } else {
	       if (cmsize > 256) {
		    cmsize = 256;
	       }
	       for (i = 0; i < cmsize; i++) {
		    *(image->rgb.red + i) = rm[i] * 256;
		    *(image->rgb.green + i) = gm[i] * 256;
		    *(image->rgb.blue + i) = bm[i] * 256;
	       }
	       image->rgb.used = cmsize;
	  }
	  op = image->data;
	  for (y = 0; y < height; y++) {
	       if (zread(zf, op, width) != width) {
		    printf("%s: short read line %d\n", image->title, y);
		    return(0); /* show what we got */
	       }
	       op += width;
	  }
     } else {
	  op = image->data;
	  skip = bands - 3;
	  linewidth = bands * width;
	  buf = (unsigned char *)malloc((unsigned)linewidth);
	  if (buf == NULL) {
	       memoryExhausted();    
	  }
	  for (y = 0; y < height; y++) {
	       ip = buf;
	       if (zread(zf, buf, linewidth) != linewidth) {
		    printf("%s: short read line %d\n", image->title, y);
		    return(0); /* show what we got */
	       }
	       for (x = 0; x < width; x++) {
		    ip += skip;
		    *(op++) = *(ip+2);
		    *(op++) = *(ip+1);
		    *(op++) = *(ip);
		    ip += 3;
	       }
	  }
	  free(buf);
     }
     
     return(0);
}

static int
parseHeader()
{
     int bits;
     int line;

     /* KISS ? */

     for (line = 0; ((line < MAX_HEAD_LINES) && (header[line] != NULL)); line++) {
	  if (strncmp(header[line], "type", 4) == 0) {
	       if (strncmp(header[line], "type raster", 11) != 0) {
		    printf("Only Sun VFF files of type `raster' are supported.\n");
		    freeheader();
		    return(1);
	       }
	  }
	  if (strncmp(header[line], "bands", 5) == 0) {
	       sscanf(header[line], "bands %d", &bands);
	  }
	  if (strncmp(header[line], "bits", 4) == 0) {
	       sscanf(header[line], "bits %d", &bits); /* only check one band */
	       if (bits != 8) {
		    printf("Only Sun VFF files with 8-bit bands are supported.\n");
		    freeheader();
		    return(1);    
	       }
	  }
	  if (strncmp(header[line],"components", 10) == 0) {
	       printf("Only Sun VFF files of type `raster' are supported.\n");
	       freeheader();
	       return(1);
	  }
	  if ((strncmp(header[line], "colormap", 8) == 0) &&
	      (strncmp(header[line], "colormapsize", 12) != 0)) {
	      {
		   char *cp;
		   int i, r, g, b;

		   /* ship colormap */
		   cp = header[line]+8;
		   /* find the first entry */
		   while (*cp && !isxdigit(*cp)) {
			cp++;
		   }
		   for (i = 0; i < 256; i++) {
			if (sscanf(cp, "%02x%02x%02x", &b, &g, &r) != 3)
			     break;
			bm[i] = b;
			gm[i] = g;
			rm[i] = r;
			cp += 6;
			while (*cp && !isxdigit(*cp)) {
			     cp++;
			}
		   }
		   cmsize = i;
	      }
 
	  }
	  if (strncmp(header[line], "size", 4) == 0) {
	       sscanf(header[line], "size %d %d", &width, &height);
	  }
     }
     freeheader();
     return(0);
}

static void
freeheader()
{
     int line;

     for (line = 0; ((line < MAX_HEAD_LINES) && (header[line] != NULL)); line++) {
	  free(header[line]);
    }
}

static int
readHeader(zf)
ZFILE *zf;
{
     char buf[HEAD_BUF_SIZE];
     int count, c, lines;
     char *cp;
     
     cp = buf;
     count = 0;
     lines = 0;

     while (lines < MAX_HEAD_LINES) {
	  while ((((c = zgetc(zf)) != '\f')) && (c != EOF) && (c != '=')) {
	       switch(c) {
	       case '\n':
	       case ' ':
	       case '\t':
	       case '\r':
		    continue;
	       
	       case ';':
		    printf("Malformed Sun VFF header line: a `;' was found before a `='.\n" );
		    return(1);
		    default:
		    if (count < HEAD_BUF_SIZE) {
			 *cp = (char)c;
			 cp++;
			 count++;
		    } else {
			 printf("Malformed Sun VFF header line: a line longer then %d bytes was found.\n", HEAD_BUF_SIZE);
			 return(0); /* go ahead and try to parse what we got (geometry file?) */
		    }
		    break;     
	       }
	  }


	  if (c == '=') {
	       if (count < HEAD_BUF_SIZE) {
		    *cp = ' ';
		    cp++;
		    count++;
	       } else {
		    printf("Malformed Sun VFF header line: a line longer then %d bytes was found.\n", HEAD_BUF_SIZE);
		    return(0);
	       
	       }
	
	       while ((c = zgetc(zf)) != ';') {
		    switch(c) {
		    case EOF:
			 printf("Malformed Sun VFF header line: a `EOF' was found before a `;'.\n" );
			 return(1);
		    case '\f':
			 printf("Malformed Sun VFF header line: a `\f' was found before a `;'.\n" );
			 return(1);
		    case '\n':
			 printf("Malformed Sun VFF header line: a `\\n' was found before a `;'.\n" );
			 return(1);
		    case '\\':
			 if ((c = zgetc(zf)) == EOF) {
			      printf("Malformed Sun VFF header line: a `EOF' was found after a `\\'.\n" );
			      return(1);
			 }
			 default:
			 if (count < HEAD_BUF_SIZE) {
			      *cp = (char)c;
			      cp++;
			      count++;
			 } else {
			      printf("Malformed Sun VFF header line: a line longer then %d bytes was found.\n", HEAD_BUF_SIZE);
			      return(0);
			 }

		    }
	       }
	       if (count < HEAD_BUF_SIZE) {
		    *cp = '\0';
		    cp++;
		    count++;
	       } else {
		    printf("Malformed Sun VFF header line: a line longer then %d bytes was found.\n", HEAD_BUF_SIZE-1);
		    return(0);
	       }

	       if ((header[lines] = (char *)malloc((unsigned)count+1)) == NULL) {
		    memoryExhausted();    
	       }
	       bcopy(buf, header[lines], count);
	       count = 0;
	       cp = buf;
	       lines++;
	  } else if (c == EOF) {
	       printf("Malformed Sun VFF header line: EOF found before the end of the header.\n");
	       return(1);  
	  } else if (c == '\f') {
	       /* read the \n after the \f */
	       c = zgetc(zf);
	       if (c == EOF) {
		    printf("Malformed Sun VFF header line: EOF found before the end of the header.\n");
		    return(1);
	       }
	       return(0);
	  }
     }
     printf("Malformed Sun VFF header: more then %d lines found.\n", MAX_HEAD_LINES);
     return(0); /* so that we try and see what was wrong */
}

vffIdent(fullname, name)
char *fullname, *name;
{

     ZFILE *zf;
     char buf[4];

     if (!(zf = zopen(fullname)))
	  return(0);

     if (zread(zf, (unsigned char *)buf, 4) != 4) {
	  zclose(zf);
	  return(0);
     }

     if (strncmp(buf, "ncaa", 4) != 0) {
	  zclose(zf);
	  return(0);
     }

     if (readHeader(zf)) {
	  zclose(zf);
	  return(0);
     }

     if (parseHeader()) {
	  zclose(zf);
	  return(0);
     }

     if (bands == 1) {
	  printf("%s is a %dx%d Sun VFF image with 1 band\n", name, width, height);
     } else {
	  printf("%s is a %dx%d Sun VFF image with %d bands\n", name, width, height, bands);
     }
     return(1);
}
