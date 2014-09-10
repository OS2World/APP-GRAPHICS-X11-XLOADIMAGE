/*
** pcx.c - load a ZSoft PC Paintbrush (PCX) file for use inside xloadimage
**
**	Tim Northrup <tim@BRS.Com>
**	Adapted from code by Jef Poskanzer (see Copyright below).
**
**	Version 0.1 --  4/25/91 -- Initial cut
**
**  Copyright (c) 1991 Tim Northrup
**	(see file "tgncpyrght.h" for complete copyright information)
*/
/*
** Copyright (C) 1988 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** This program (pcxtopbm) is based on the pcx2rf program by:
**   Mike Macgirvin
**   Stanford Relativity Gyro Program GP-B
**   Stanford, Calif. 94503
**   ARPA: mike@relgyro.stanford.edu
*/

#include <stdio.h>
#include "image.h"
#include "tgncpyrght.h"
#include "pcx.h"

/*
**  pcxIdent
**
**	Identify passed file as a PC Paintbrush image or not
**
**	Returns 1 if file is a PCX file, 0 otherwise
*/


unsigned int pcxIdent ( fullname, name )
	char *fullname, *name;
{
	ZFILE *zf;
	unsigned int ret;
	int xmin;
	int xmax;
	int ymin;
	int ymax;
	int colors;
	char *type;

	ret = 0;
	if (! (zf = zopen ( fullname )))
	    return ( 0 );
	PCXH = (PCXHeader *) lmalloc ( PCXHsize );
	if (zread ( zf, (byte *)PCXH, PCXHsize ) == PCXHsize) {
	    if ((PCXH->Zid == PCX_MAGIC) && (PCXH->Zver <= 5)) {
		xmin = Word ( PCXH->Zxminlo, PCXH->Zxminhi); 
		xmax = Word ( PCXH->Zxmaxlo, PCXH->Zxmaxhi); 
		ymin = Word ( PCXH->Zyminlo, PCXH->Zyminhi); 
		ymax = Word ( PCXH->Zymaxlo, PCXH->Zymaxhi); 
		xmax = xmax - xmin + 1;
		ymax = ymax - ymin + 1;
		colors = 1 << (PCXH->Zbpp * PCXH->Znplanes);
		type = " PC Paintbrush image\n";
		if (colors == 2) 
		     printf ( "%s is a %dx%d monochrome%s",
				name, xmax, ymax, type );
		else printf ( "%s is a %dx%d %d color%s",	
		   	 name, xmax, ymax, colors, type );
		ret = 1;
	    }
	}
	zclose ( zf );
	lfree ( (byte *)PCXH );
	return ( ret );
}


/*
**  PCX_Load_Raster
**
**	Load raster image data into passed image structure. Raster Data
**
**	means : Every bit is a pixel (if depth is 1) or every byte is a 
**
**	pixel (if depth is 8).
**
**	Returns no value (void function)
*/

static void PCX_Load_Raster ( zf, image, depth )
	ZFILE *zf;
	Image *image;
	int depth;
	/* Assertion : depth is 1 or 8 */
{
	byte *ptr	= &(image->data[0]);
	int row		= 0;
	int by_this_row = 0;
	int by_per_row;
	int linelen;
	int corrector;
	int i, b, cnt;

	by_per_row   = Word ( PCXH->Zbprlo, PCXH->Zbprhi);
	if (depth == 1)
	    linelen  = (image->width / 8) + (image->width % 8 ? 1 : 0);
	else linelen = image->width;
	corrector    = by_per_row - linelen;
	/* bytes per row is always even, which means that there is an	*/
	/* excess byte if an odd nr of bytes would be sufficient.	*/
	/* As newBitImage allocated memory without this	excess, we have */
	/* to read one less. But as there are two cases of reading,	*/
	/* inside and outside of a run, I correct it afterwards, when	*/
	/* the line is complete.					*/
	while ((b = zgetc(zf)) != EOF) { 	/* Are we done ?	*/
	    if ((b & 0xC0) == 0xC0) {		/* have a rep. count	*/
		cnt = b & 0x3F;			/* mask rep. bits out	*/
		b = zgetc ( zf );		/* get real bits	*/
		if (b == EOF) {			/* Shouldn't happen !	*/
		    printf ( "Unexpected EOF\n" );
		    return;
		}
	    }
	    else cnt = 1;			/* no repeating this one*/
	    if (depth ==1) b = 255 - b;		/* Have to invert	*/
	    for ( i = 0; i < cnt; i++ ) {	/* store count times	*/
		*ptr++ = (byte) b;
		if (++by_this_row == by_per_row) {
		    row++;			/* start of a new line	*/
		    by_this_row = 0;		/* reset counter	*/
		    if (corrector) ptr--;	/*evtlly correct pointer*/
		    if ( row >= image->height ) {
#if 0
			/* happens a lot on valid images - jimf 10.28.91 */
			if (depth == 1)		/* Color : Map follows	*/
			    printf ("Warning: excess data ignored\n");
#endif
			return;
		    }
		}
	    }
	}
}    

/*
**  PCX_Planes
**
**	Load plane image data into passed image structure. Plane data
**
**	means : There are N planes, each containing M bits of a pixel
**
**	byte, where N * M is not greater than 8 and M divides 8. (The
**
**	cases M = 1 or 8, N = 1 are covered by PCX_Load_Raster).
**
**	Returns no value (void function)
*/


static void PCX_Planes ( zf, image, bpp, nr_pl )
	ZFILE *zf;
	Image *image;
	int bpp, nr_pl;
	/* Assertion : 							*/
	/* bpp is 1, 2 or 4 only, dividing 8 without remainder anyways,	*/
	/* bpp * nr_pl <= 8						*/
{
	byte *ptr, *sptr;
	register byte *tptr;	
	int row		= 0;
	int by_this_row = 0;
	int by_per_row;
	int this_plane	= 0;
	int shifter	= 0;
	register int j;
	int i, b, cnt;
			
	ptr = &(image->data[0]);
	by_per_row  = Word ( PCXH->Zbprlo, PCXH->Zbprhi);
	sptr = tptr = (byte *) lcalloc ( by_per_row*8 );
	/* We can't correct as easy as above, because we handle several	*/
	/* bit planes simultaneously, and we must not load beyond row	*/
	/* limits. So we load into a temporary row and copy it into	*/
	/* image structure when the row is completed.			*/
	while ((b = zgetc(zf)) != EOF) { 
	    if ((b & 0xC0) == 0xC0) {	/* Get count and data as above	*/
		cnt = b & 0x3F;	
		b = zgetc ( zf );
		if (b == EOF) {
		    printf ( "Unexpected EOF\n" );
		    return;
		}
	    }
	    else cnt = 1;
	    for ( i = 0; i < cnt; i++ ) {	/* Load data into temp.	*/
		switch (bpp) {
		    case 1 : 
			*tptr++ |= (byte) (((b & 0x80 ) >> 7) << shifter);
			*tptr++ |= (byte) (((b & 0x40 ) >> 6) << shifter);
			*tptr++ |= (byte) (((b & 0x20 ) >> 5) << shifter);
			*tptr++ |= (byte) (((b & 0x10 ) >> 4) << shifter);
			*tptr++ |= (byte) (((b & 0x08 ) >> 3) << shifter);
			*tptr++ |= (byte) (((b & 0x04 ) >> 2) << shifter);
			*tptr++ |= (byte) (((b & 0x02 ) >> 1) << shifter);
			*tptr++ |= (byte)  ((b & 0x01 )       << shifter);
			/* This is not a loop for performance reasons.	*/
			/* Can't write e.g. ">> 2 - shifter", because 	*/
			/* that expression would get negative.		*/
			break;
		    case 2 :
			*tptr++ |= (byte) (((b & 0xC0 ) >> 6) << shifter);
			*tptr++ |= (byte) (((b & 0x30 ) >> 4) << shifter);
			*tptr++ |= (byte) (((b & 0x0C ) >> 2) << shifter);
			*tptr++ |= (byte)  ((b & 0x03 )       << shifter);
			break;
		    case 4 :
			*tptr++ |= (byte) (((b & 0xF0) >> 4) << shifter);
			*tptr++ |= (byte)  ((b & 0x0F)       << shifter);
			break;
		    default :; /* Can't happen if assertion holds	*/
		}
		if (++by_this_row == by_per_row) {	/* Row done ?	*/
		    by_this_row = 0;		 /* reset counter	*/
		    if (++this_plane == nr_pl) { /* was it last plane ?	*/
			row++;			 /* inc counter		*/
			tptr = sptr;		 /* get saved ptr	*/
			this_plane = shifter = 0;/* reset plane Nr	*/
			for (j = 0; j < image->width; j++) {
			    *ptr++  = *tptr;	/* store final data	*/
			    *tptr++ = 0;	/* clear temp data	*/
			}
			if ( row >= image->height ) 
			    return;		/* Done with image ?	*/
		    }
		    else			/* Prepare next plane	*/
			shifter = this_plane * bpp;
		    tptr = sptr;		/* Get saved ptr	*/
		}
	    }
	}
}  


/*
**  PCX_LoadImage
**
**	Load PC Paintbrush file into the passed Image structure.
**
**	Returns no value (void function)
*/


static void PCX_LoadImage ( zf ,image )
	ZFILE *zf;
	Image *image;
{
	int pl;
	switch (PCXH->Zbpp) {	/* What kind of plane do we have ?	*/
	    case 1 :			/* Bit planes			*/
		if (PCXH->Znplanes == 1)	/* Only one : Read it	*/
		    PCX_Load_Raster ( zf, image, 1 );
		else PCX_Planes ( zf, image, 1, PCXH->Znplanes );
		break;
	    case 2 :			/* Two or four bits per plane	*/
	    case 4 :			/*      are read plane by plane */
		PCX_Planes ( zf, image, PCXH->Zbpp, PCXH->Znplanes );
		break;
	    case 8 :			/* Byte planes			*/
		if (PCXH->Znplanes == 1)	/* Only one : Read it	*/
		    PCX_Load_Raster ( zf, image, 8 );
		else {				/* More not allowed	*/
		    printf ("Only 1 plane allowed if 8 bits per plane\n");
		    exit (1);
		}
		break;
	    default :				/* Neither case found	*/
		printf ("%d bits per plane not supported\n", PCXH->Zbpp );
		exit (1);
	}
}


/*
**  pcxLoad
**
**	Load PCX Paintbrush file into an Image structure.
**
**	Returns pointer to allocated struct if successful, NULL otherwise
*/

Image *pcxLoad ( fullname, name, verbose )
	char *fullname, *name;
	unsigned int verbose;
{
	ZFILE *zf;
	unsigned int i, ret;
	int xmin;
	int xmax;
	int ymin;
	int ymax;
	int colors;
	PCXcolor *cmap;
	int clen;
	Image *image;
	
	if ( ! (zf = zopen ( fullname )))	/* Open input file	*/
	    return ( (Image *) NULL);
	PCXH = (PCXHeader *) lmalloc ( PCXHsize );
	if (zread ( zf, (byte *)PCXH, PCXHsize ) != PCXHsize) {	/* Read header	*/
	    zclose ( zf );
	    return ( (Image *) NULL );
	}
	if ((PCXH->Zid != PCX_MAGIC) || (PCXH->Zver > 5)) {	
	    zclose ( zf );		/* Is it PCX, Version less 5 ?	*/
	    return ( (Image *) NULL );
	}
	znocache(zf);			/* don't need caching anymore	*/
	xmin = Word ( PCXH->Zxminlo, PCXH->Zxminhi); /* Calculate sizes	*/
	xmax = Word ( PCXH->Zxmaxlo, PCXH->Zxmaxhi); 
	ymin = Word ( PCXH->Zyminlo, PCXH->Zyminhi); 
	ymax = Word ( PCXH->Zymaxlo, PCXH->Zymaxhi); 
	xmax = xmax - xmin + 1;
	ymax = ymax - ymin + 1;	
	colors = 1 << (PCXH->Zbpp * PCXH->Znplanes); /* Calculate colors*/
	if (verbose) {				/* Print Information	*/
	     if (colors == 2) 
		printf ( "%s is a %dx%d monochrome PC Paintbrush image\n",
				name, xmax, ymax );
	     else printf ( "%s is a %dx%d %d color PC Paintbrush image\n",
				name, xmax, ymax, colors );
	}
	if (colors > 256) {
	    printf ( "No more than 256 colors allowed in PCX format\n" );
	    exit (1);
	}
	if (PCXH->Zenc == 0) {
	    printf ( "Unencoded PCX format not yet supported. Please" );
	    printf ( " email the uuencoded image\n to erueg@cfgauss." );
	    printf ( "uni-math.gwdg.de\n" );
	    exit (1);
	}
	if (colors == 2)	/* Allocate appropriate pbm array	*/
	    image = newBitImage ( xmax, ymax );
	else {
	    image = newRGBImage ( xmax, ymax, 8 );	
	}
	PCX_LoadImage ( zf, image );
	if (colors > 16) {		/* Handle external colormap	*/
	    while ((i = zgetc(zf)) != PCX_MAPSTART);
	    clen = colors * 3;
	    cmap = (PCXcolor *) lmalloc ( clen );
	    if (zread ( zf, (byte *)cmap, clen ) != clen) {
		printf ( "EOF while reading colormap" );
		exit (1);
	    }
	    for ( i = 0; i < colors; i++) {
		*(image->rgb.red   + i) = (cmap [i].Zred   << 8);
		*(image->rgb.green + i) = (cmap [i].Zgreen << 8);
		*(image->rgb.blue  + i) = (cmap [i].Zblue  << 8);
	    }
	    image->rgb.used = colors;
	    lfree ( (byte *)cmap );
	}
	else if (colors > 2) {		/* Handle internal colormap	*/
	    for ( i = 0; i < colors; i++) {
		*(image->rgb.red   + i) = (PCXH->Zcmap [i].Zred   << 8);
		*(image->rgb.green + i) = (PCXH->Zcmap [i].Zgreen << 8);
		*(image->rgb.blue  + i) = (PCXH->Zcmap [i].Zblue  << 8);
	    }
	    image->rgb.used = colors;
	}
	zclose ( zf );	
	lfree ( (byte *)PCXH );
	image->title = dupString(name);
	return ( image );
}
