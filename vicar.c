/*
** vic.c - load a VICAR Image file for use inside xloadimage
**
** Khalid Soofi (August 1992)
*/

#define TRUE      1
#define FALSE     0
#define DEPTH     8
#define LBLSIZE   label[0]
#define LINES     label[8]
#define SAMPLES   label[6]
 
#include <stdio.h>
#include <malloc.h>
#include "image.h"
#include <sys/types.h>

FILE *fp,*fopen();
int  label[10];
char labstr[80];
int  flag=0;



/*
** vicarIdent
**
** Identify passed file a VICAR Image
**   return 1 for true
**   return 0 otherwise
**
*/

unsigned int vicarIdent (fullname, name)
     char *fullname, *name;
{
  int i=0;
  char IDENT[7];
  char c;

  if ( ! (fp = fopen(fullname,"r")) ) 
    return(0);

  /* identify the VICAR IMAGE */

  fscanf(fp,"%6s",IDENT);
  if ( strcmp(IDENT,"LBLSIZ") == 0 ) {
     printf("%s is a VICAR Image: ",fullname);
     fclose(fp);
  }
  else 
     return(0);

  /* Fill up the label block of image */

  fp = fopen(fullname,"r");

  while ( i < 10) {
    fscanf(fp,"%c",&c);
    if ( c == '=' )
      fscanf(fp,"%d",&label[i++]);
  }

  printf("%d lines and %d samples\n", LINES , SAMPLES );
  fclose(fp);
  return(1);
}
         

/* vicHeader - print the history record */

static void vicarHeader(ptr)
     char *ptr;
{
  int i;
  int prflag;
  int rtflag;

  printf("\n ---HISTORY RECORD------------------------------------\n");
  for ( i=0, prflag=FALSE, rtflag=0 ; i < LBLSIZE ; i++ ) {
    if ( *(ptr++) == '\'' ) {
      prflag = !prflag;
      rtflag++;
    }
    if ( prflag && (char)(*ptr) != '\'' )
      printf("%c",(char)(*ptr));
    if ( rtflag == 2 ) {
      printf("\n");
      rtflag = 0;
    }
    
  }
  printf("\n -----------------------------------------------------\n");
}


/*
** vicarLoad
**
** Load Vicar image into an Image structure.
**
** Return pointer to allocated struct if successful, NULL otherwise
**
*/

Image *vicarLoad(fullname, name, verbose)
     char *fullname, *name;
     unsigned int verbose;
{
  char         c;
  int          i,j;
  unsigned int mapsize, size;
  Image        *image;
  byte         *lineptr;
  byte         *map;
  byte         *mapred,*mapgreen,*mapblue;
  byte         *buf;
  ZFILE        *zf;
 
  if ( vicarIdent(fullname,name,verbose) == 0 )
    return(0);

  /* define the image structure */

  image = newRGBImage( SAMPLES , LINES , DEPTH );
  image->width = SAMPLES ;
  image->height = LINES ;
  image->depth = DEPTH ;
  image->title = dupString(name);

 /* 
  * Set up the grey scale lookup table:-
  *   From  Jim frost 09.27.89 (sunraster.c) 
  *   Copyright 1989, 1991 Jim Frost.
  *   See included file "copyright.h" for complete copyright information.
  */

  mapsize = 256*3; 
  map= lmalloc(mapsize); 
  for (i = 0; i < 256; i += 1) { 
   map[i] = map[256+i] = map[2*256+i] = i;
  }
  mapsize /= 3;
  mapred= map;
  mapgreen= mapred + mapsize;
  mapblue= mapgreen + mapsize;
  if (image->rgb.size == 0)
      newRGBMapData(&image->rgb, mapsize);
  for (i= 0; i < mapsize; i++) {
    *(image->rgb.red + i)= (*(mapred++) << 8);
    *(image->rgb.green + i)= (*(mapgreen++) << 8);
    *(image->rgb.blue + i)= (*(mapblue++) << 8);
  }
  lfree(map);
  image->rgb.used= mapsize;
 
 /* 
  * Finally start the image data
  */

  lineptr = image->data;

  if ( !(zf=zopen(fullname))) {
     perror(fullname);
     zclose(zf);
     exit(1);
  }

  if ( verbose ){
    zread(zf,lineptr,LBLSIZE);                     /* dump label */
    vicarHeader(lineptr);
  }
  else
    zread(zf,lineptr,LBLSIZE);                     /* skip label */

  size = image->width * image->height;
  if (zread(zf,lineptr,size) != size)
    fprintf(stderr, "%s: Warning, image data was missing\n", fullname);
  zclose(zf);
  return(image);

}
