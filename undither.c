/* undither.c:
 *
 * this converts a bitmap image into a greyscale image.
 *
 * Eckhard R"uggeberg (erueg@cfgauss.uni-math.gwdg.de).
 */

#include "image.h"

Image *undither(oimage, verbose)
     Image *oimage;
     unsigned int  verbose;
{
  Image *nimage;			/* new image to build		*/
  byte *optr, *nptr;		/* Ptr into old/new data	*/
  int i, j, k;			/* Loop counters		*/ 
  int hi, lo;			/* 2 new Pixels constructed	*/
  register int scanned;		/* byte scanned in old data	*/
  int newX, newY;			/* new sizes			*/
  int bitskip, byteskip;		/* skips for ptr in old data	*/
  int scantimes;			/* how often to scan a byte	*/
  int oddness;			/* If newX is odd		*/

  if (!BITMAPP(oimage))
    return(oimage);
  if (verbose) {
    printf ( "  Undithering image into grayscale..." );
    fflush(stdout);
  }
  newX = (oimage->width  / 4);
  oddness = newX & 1; 
  newY = oimage->height / 4;
  nimage = newRGBImage ( newX, newY, 8 );
  if (oimage->title) {
    nimage->title = (char *) lmalloc ( strlen (oimage->title) + 13 );
    sprintf ( nimage->title, "%s (grayscaled)", oimage->title );
  }
  nimage->rgb.used = 17;
  for ( i = 0; i < 17; i++ )
    *(nimage->rgb.red + i) = *(nimage->rgb.green + i) = 
      *(nimage->rgb.blue + i) = 65535.0 * (16 - i) / 16.0;
  bitskip   = (oimage->width / 8) + (oimage->width % 8 ? 1 : 0);
  byteskip  = (bitskip << 2);
  scantimes = (newX >> 1) + oddness; 
  optr = oimage->data;
  nptr = nimage->data;
  for ( i = 0; i < newY; i++ ) {		/* for each new row	*/
    for ( j = 0; j < scantimes; j++ ) {	/* for each new colunm	*/
      lo = hi = 0;			
      for ( k = 0; k < 4; k++ ) {	/* for each of the 4	*/
	scanned = *(optr + j + k*bitskip);	/*     old rows */
	if (scanned & 0x80) hi++;
	if (scanned & 0x40) hi++;
	if (scanned & 0x20) hi++;
	if (scanned & 0x10) hi++;
	if (scanned & 0x08) lo++;
	if (scanned & 0x04) lo++;
	if (scanned & 0x02) lo++;
	if (scanned & 0x01) lo++;
      }
      *nptr++ = hi;			/* store new data	*/
      *nptr++ = lo;
    }
    if (oddness) nptr--;		/* correct at end of row*/
    optr += byteskip;
  }
  if (verbose)
    printf("done\n");
  return(nimage);
}
