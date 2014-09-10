/* mcidas.c:
 *
 * McIDAS areafile support.  contributed by Glenn P. Davis
 * (davis@unidata.ucar.edu).
 */

#include "xloadimage.h"
#include "mcidas.h"

char *mc_sensor();

static struct {
  int   days;
  char *name;
} month_info[13] = {
  { 0,  "Pad" },
  { 31, "Jan" },
  { 29, "Feb" }, /* longest */
  { 31, "Mar" },
  { 30, "Apr" },
  { 31, "May" },
  { 30, "Jun" },
  { 31, "Jul" },
  { 31, "Aug" },
  { 30, "Sep" },
  { 31, "Oct" },
  { 30, "Nov" },
  { 31, "Dec" }
};

/* convert numeric dates to human-readable
 */
static char *convert_date(time, date)
     int time, date;
{ static char buf[30];
  int hour;
  int minute;
  int second;
  int year;
  int month;
  int day;

  year = date / 1000;
  if (year > 50) /* gives us a range from 1950 - 2050, probably good enough */
    year += 1900;
  else
    year += 2000;
  day = date % 1000;

  /* adjust day for non-leap-years if necessary
   */
  if ((day > 31 + 28) && ((year % 4) || (year == 2000)))
    day++;

  /* find month in year that this day falls in
   */
  for (month= 1; day > month_info[month].days; day -= month_info[month].days)
    month++;

  /* break time-of-day up
   */
  hour = time / 10000;
  minute = (time % 10000) / 100;
  second = (time % 100);

  sprintf(buf, "%d:%2.2d:%2.2d %s %d, %d (day %d)",
	  hour, minute, second, month_info[month].name, day, year,
	  (date % 1000));
  return(buf);
}

/*
 * convert from little endian to big endian four byte object
 */
static unsigned long
vhtonl(lend)
unsigned long lend ;
{
	unsigned long bend ;
	unsigned char *lp, *bp ;

	lp = ((unsigned char *)&lend) + 3 ;
	bp = (unsigned char *) &bend ;

	*bp++ = *lp-- ;
	*bp++ = *lp-- ;
	*bp++ = *lp-- ;
	*bp = *lp ;

	return(bend) ;
}

static void babble(name, dir)
     char *name;
     struct area_dir *dir;
{
  printf("%s is a %dx%d McIDAS areafile from %s at %s (%d, %d) (%d, %d)\n",
	 name,
	 dir->esiz, dir->lsiz,
	 mc_sensor(dir->satid),
	 convert_date(dir->itime, dir->idate),
	 dir->lcor,
	 dir->ecor,
	 dir->lres,
	 dir->eres) ;
}

static void swap_bytes(dir)
     struct area_dir *dir;
{
  unsigned long *begin ; 
  unsigned long *ulp ;

  begin = (unsigned long *)dir ;
  for(ulp = begin ; ulp < &begin[AREA_COMMENTS] ; ulp++)
    *ulp = vhtonl(*ulp) ;
  for(ulp = &begin[AREA_CALKEY] ; ulp < &begin[AREA_STYPE] ; ulp++)
    *ulp = vhtonl(*ulp) ;
}

/* ARGSUSED */
int mcidasIdent(fullname, name)
     char *fullname, *name;
{ ZFILE          *zf;
  struct area_dir dir ;
  int             r;

  if (! (zf= zopen(fullname))) {
    perror("mcidasIdent");
    return(0);
  }
  switch (zread(zf, (byte *)&dir, sizeof(struct area_dir))) {
  case -1:
    perror("mcidasIdent");
    r= 0;
    break;

  case sizeof(struct area_dir):
    if (memToValLSB((char *)&dir.type, 4) != 4) {
      r= 0;
      break;
    }
    if (dir.type != 4)
      swap_bytes(&dir);
    babble(name, &dir);
    r= 1;
    break;

  default:
    r= 0;
    break;
  }
  zclose(zf);
  return(r);
}


Image *mcidasLoad(fullname, name, verbose)
     char         *fullname, *name;
     unsigned int  verbose;
{ ZFILE          *zf;
  struct area_dir  dir;
  struct navigation  nav;
  Image          *image;
  unsigned int    y;
  int doswap = 0 ;

  if (! (zf= zopen(fullname))) {
    perror("mcidasLoad");
    return(NULL);
  }
  switch (zread(zf, (byte *)&dir, sizeof(struct area_dir))) {
  case -1:
    perror("mcidasLoad");
    zclose(zf);
    exit(1);

  case sizeof(struct area_dir):
    if (memToValLSB((char *)&dir.type, 4) != 4) {
      zclose(zf);
      return(NULL) ;
    }
    if (dir.type != 4)
      swap_bytes(&dir);
    break;

  default:
    zclose(zf);
    return(NULL);
  }

  if (verbose)
    babble(name, &dir);

  znocache(zf);
  /* skip the nav */
  if( zread(zf, (byte *)&nav, sizeof(struct navigation)) !=
     sizeof(struct navigation)) {
      zclose(zf);
      return(NULL) ;
  }

  /* get an image to put the data in
   */
  image= newRGBImage(dir.esiz, dir.lsiz, dir.zsiz * 8);

  /* set up the colormap, linear grey scale
   */
  for (y= 0; y < image->rgb.size; y++) {
      *(image->rgb.red + y)= 
	  *(image->rgb.green + y)=
	      *(image->rgb.blue + y)= y * (65536 / image->rgb.size) ;
  }
  image->rgb.used= image->rgb.size ;

  /* read the first band from the image and warn if there are other bands
   * we can't read.
   */
  zread(zf, image->data, dir.esiz * dir.lsiz * dir.zsiz) ;
  if (dir.bands > 1)
      printf("Warning: Only showing first of %d bands\n", dir.bands);

  zclose(zf);
  image->title= dupString(name);
  return(image);
}
