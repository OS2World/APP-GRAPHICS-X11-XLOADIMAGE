/* misc.c:
 *
 * miscellaneous funcs
 *
 * jim frost 10.05.89
 *
 * Copyright 1989, 1990, 1991 Jim Frost.
 * See included file "copyright.h" for complete copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"
#ifdef VMS
#include "patchlevel."
#else
#include "patchlevel"
#endif
#include <signal.h>

extern int      _Xdebug;
extern char    *ProgramName;
extern Display *Disp;
extern int      Scrn;
extern char    *BuildDate;
extern char    *BuildUser;
extern char    *BuildSystem;

static char *signalName(sig)
     int sig;
{ static char buf[32];

  switch (sig) {
  case SIGSEGV:
    return("SEGV");
  case SIGBUS:
    return("BUS");
  case SIGFPE:
    return("FPE");
  case SIGILL:
    return("ILL");
  default:
    sprintf(buf, "Signal %d", sig);
    return(buf);
  }
}

void memoryExhausted()
{
  fprintf(stderr,
	  "\n\nMemory has been exhausted; operation cannot continue (sorry).\n");
  if (_Xdebug)
    abort();
  else
    exit(1);
}

void internalError(sig)
     int sig;
{ static int handling_error= 0;
  int a, b;
  Screen *screen;

  switch(handling_error++) {
  case 0:
    printf("\n\n\
An internal error (%s) has occurred.  If you would like to file a bug\n\
report, please send email to %s\n\
with a description of how you triggered the bug, the output of xloadimage\n\
before the failure, and the following information:\n\n", signalName(sig),
	   AUTHOR_EMAIL);
    printf("Xloadimage Version %s.%s\n\n", VERSION, PATCHLEVEL);
    if (BuildUser)
      printf("Built by:     %s\n", BuildUser);
    if (BuildDate)
      printf("Built on:     %s\n", BuildDate);
    printf("Build system: %s\n", BuildSystem);
    
    if (Disp) {
      screen= ScreenOfDisplay(Disp, Scrn);
      printf("Server: %s Version %d\n", ServerVendor(Disp), VendorRelease(Disp));
      printf("Depths and visuals supported:\n");
      for (a= 0; a < screen->ndepths; a++) {
	printf("%2d:", screen->depths[a].depth);
	for (b= 0; b < screen->depths[a].nvisuals; b++)
	  printf(" %s", nameOfVisualClass(screen->depths[a].visuals[b].class));
	printf("\n");
      }
    }
    else
      printf("[No information on server; error occurred before connection]\n");
    break;
  case 1:
    fprintf(stderr, "\n\n\
An internal error has occurred within the internal error handler.  No more\n\
information about the error is available, sorry.\n");
    exit(1);
    break;
  }
  if (_Xdebug) /* dump core if -debug is on */
    abort();
  exit(1);
}

void version()
{
  printf("Xloadimage version %s.%s by Jim Frost.\n",
	 VERSION, PATCHLEVEL);
  printf("Built on %s\n", BuildSystem);
  printf("Please send email to %s for\npraise or bug reports.\n",
	 AUTHOR_EMAIL);
}

void usageHelp()
{
  printf("\nUsage: %s [global options] {[image options] image_name ...}\n\n",
	 tail(ProgramName));
  printf("\
Type `%s -help [option ...]' for information on a particular option, or\n\
`%s -help' to enter the interactive help facility.\n",
	 tail(ProgramName), tail(ProgramName));
  exit(1);
}

void usage()
{
  version();
  usageHelp();
  /* NOTREACHED */
}

char *tail(path)
     char *path;
{ int   s;
  char *t;

  t= path;
  for (s= 0; *(path + s) != '\0'; s++)
    if (*(path + s) == '/')
      t= path + s + 1;
  return(t);
}

/* simple error handler.  this provides us with some kind of error recovery.
 */

int errorHandler(disp, error)
     Display *disp;
     XErrorEvent *error;
{ char errortext[BUFSIZ];

  XGetErrorText(disp, error->error_code, errortext, BUFSIZ);
  fprintf(stderr, "xloadimage: X Error: %s on 0x%x\n",
	  errortext, error->resourceid);
  if (_Xdebug) /* if -debug mode is enabled, dump a core when we hit this */
    abort();
  else
    return(0);
}

/*
  findstr - public-domain implementation of standard C 'strstr' library
            function (renamed and slightly modified to avoid naming
            conflicts - jimf)

  last edit:	02-Sep-1990	D A Gwyn

  This is an original implementation based on an idea by D M Sunday,
  essentially the "quick search" algorithm described in CACM V33 N8.
  Unlike Sunday's implementation, this one does not wander past the
  ends of the strings (which can cause malfunctions under certain
  circumstances), nor does it require the length of the searched
  text to be determined in advance.  There are numerous other subtle
  improvements too.  The code is intended to be fully portable, but in
  environments that do not conform to the C standard, you should check
  the sections below marked "configure as required".  There are also
  a few compilation options, as follows:
*/

#define BYTE_MAX 255

#define EOS '\0'		/* C string terminator */

char *					/* returns -> leftmost occurrence,
					   or null pointer if not present */
findstr( s1, s2 )
     char	*s1;		/* -> string to be searched */
     char	*s2;		/* -> search-pattern string */
{
  register byte	*t;		/* -> text character being tested */
  register byte	*p;		/* -> pattern char being tested */
  register byte	*tx;		/* -> possible start of match */
  register unsigned int	m;      /* length of pattern */
  register byte	*top;		/* -> high water mark in text */
  unsigned int  shift[BYTE_MAX + 1];	/* pattern shift table */

  if ( s1 == NULL || s2 == NULL )
    return NULL;		/* certainly, no match is found! */

  /* Precompute shift intervals based on the pattern;
     the length of the pattern is determined as a side effect: */

  bzero(&shift[1], (BYTE_MAX * sizeof(unsigned int)));

  /* Note: shift[0] is undefined at this point (fixed later). */

  for ( m = 1, p = (byte *)s2; *p != EOS; ++m, ++p )
    shift[(byte)*p] = m;

  {
    register byte c;

    c = BYTE_MAX;
    do
      shift[c] = m - shift[c];
    while ( --c > 0 );
    /* Note: shift[0] is still undefined at this point. */
  }

  shift[0] = --m; 		/* shift[EOS]; important details! */

  /* Try to find the pattern in the text string: */

  for ( top = tx = (byte *)s1; ; tx += shift[*(top = t)] ) {
    for ( t = tx, p = (byte *)s2; ; ++t, ++p ) {
      if ( *p == EOS )       /* entire pattern matched */
	return (char *)tx;
      if ( *p != *t )
	break;
    }
    if ( t < top ) /* idea due to ado@elsie.nci.nih.gov */
      t = top;	   /* already scanned this far for EOS */
    do	{
      if ( *t == EOS )
	return NULL;	/* no match */
    } while ( ++t - tx != m );	/* < */
  }
}
