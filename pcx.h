/* pcx.h:
 *
 * PCX header file
 *
 * Eckhard R"uggeberg 15.10.91
 */

typedef struct {
	byte Zred;
	byte Zgreen;
	byte Zblue;
} PCXcolor;

typedef struct {
	byte Zid;			/* Id byte : 0x0a = ZSoft Z	*/
	byte Zver;			/* Version :	0 = 2.5
							2 = 2.8 w palette
							3 = 2.8 w/o pal.
							5 = 3.0 */
	byte Zenc;			/* Encoding :	0 = uncompressed, 
							1 = compressed	*/
	byte Zbpp;			/* Bits per pixel per plane, 
						recommended only 1 or 8 */
	byte  Zxminlo; byte Zxminhi;	/* left  Border Pixel coord.	*/
	byte  Zyminlo; byte Zyminhi;	/* upper Border Pixel coord.	*/
	byte  Zxmaxlo; byte Zxmaxhi;	/* right Border Pixel coord.	*/
	byte  Zymaxlo; byte Zymaxhi;	/* lower Border Pixel coord.	*/
	short Zhres;			/* horiz. resolution, ignored	*/
	short Zvres;			/* vert.  resolution, ignored	*/
	PCXcolor Zcmap [16];		/* Colormap for 16-color pict.	*/
	byte  Zreserved;			
	byte  Znplanes;			/* # of planes, recommended 1,4 */
	byte  Zbprlo;	  byte Zbprhi;	/* bytes per row (always even)	*/
	short  Zpalinfo;		/* Palette info, ignored	*/
	byte  Zfiller[58];		/* reserved Filler = 0		*/
} PCXHeader;

static PCXHeader *PCXH;			/* To avoid a parameter for all */
					/*		     procedures */
#define Word(LO,HI) (HI << 8) + LO	/* to swap bytes in a word	*/
#define PCX_MAGIC 0x0a			/* first byte in a PCX file	*/
#define PCX_MAPSTART 0x0c		/* Start of appended colormap	*/
#define PCXHsize 128			/* Size of PCX Header		*/
