/* config.c:
 *
 * functions that deal with the particular configuration.
 *
 * jim frost 10.03.89
 *
 * Copyright 1989, 1990, 1991, 1993 Jim Frost.
 * See included file "copyright.h" for complete copyright information.
 */

#include "copyright.h"
#include <ctype.h> /* must here for debugging version under AIX */
#include "xloadimage.h"
#include <X11/Xos.h>
#include <sys/stat.h>
#ifndef VMS
#include <pwd.h>
#endif
#include <errno.h>
#ifndef IS_BSD
#include <unistd.h>
#endif

/* SUPPRESS 530 */
/* SUPPRESS 560 */

extern int errno;

struct filter *Filters = (struct filter *)NULL;

static unsigned int  NumPaths= 0;
static unsigned int  NumExts= 0;
static char         *Paths[BUFSIZ];
static char         *Exts[BUFSIZ];
static char         *PathToken= "path";
static char         *ExtToken= "extension";
static char         *FilterToken= "filter";

static void addFilter(extension, filter)
     char *extension, *filter;
{ struct filter *f, *t;

  f= (struct filter *)lmalloc(sizeof(struct filter));
  f->extension= dupString(extension);
  f->filter= dupString(filter);
  f->next= NULL;
  if (Filters) {
    for (t= Filters; t->next; t= t->next)
      /* EMPTY */
      ;
    t->next= f;
  }
  else
    Filters= f;
}

enum parse_state {
  parse_void,
  parse_state_token,
  parse_path,
  parse_extension,
  parse_filter_name,
  parse_filter_extension
};

/* this function eats up whitespace, incrementing the line number at
 * each newline
 */
static void skip_whitespace(f, linenum)
     FILE *f;
     unsigned int *linenum;
{
  int c;

  for (;;) {
    c = fgetc(f);
    switch (c) {
    case EOF:
      return;
    case '\n':
      (*linenum)++;
      /* FALLTHRU */
    case '\r':
    case ' ':
    case '\t':
      continue;
    default:
      ungetc(c, f);
      return;
    }
  }
}


static void readPathsAndExts(name)
     char *name;
{ FILE         *f;
  char          buf[BUFSIZ];
  char          filter_name[BUFSIZ];
  enum parse_state state;
  unsigned int  linenum;
  unsigned int  a;
  int           c;

  if (! (f= fopen(name, "r")))
    return;

  state = parse_void;
  linenum= 0;

  /* this is the token scanner.  i suppose i could have used lex.
   */
  a = 0;
  for (;;) {
    c = fgetc(f);
    if (a >= BUFSIZ) {
      fprintf(stderr, "%s: %d: Buffer overflow (token too long, sorry).\n", name, linenum);
      fclose(f);
      return;
    }
    switch (c) {
    case EOF:
      return;
    case '\n': /* increment line count */
      linenum++;
      /* FALLTHRU */
    case '\r': /* treated as generic white space */
    case ' ':
    case '\t':
      buf[a] = '\0';
      a = 0;
      /* skip forward until next character */
      skip_whitespace(f, &linenum);
      c = fgetc(f);
      if (c == '=') {
	state = parse_state_token;
	skip_whitespace(f, &linenum);
      }
      else if (c != EOF)
	ungetc(c, f);
      break;
    case '=':
      buf[a] = '\0';
      a = 0;
      state = parse_state_token;
      skip_whitespace(f, &linenum);
      break;
    case '#': /* comment */
      for (;;) {
	c = fgetc(f);
	switch (c) { /* eat everything to newline or EOF */
	case '\n':
	  linenum++;
	case EOF:
	  break;
	default:
	  continue;
	}
	break;
      }

      /* terminate token
       */
      buf[a] = '\0';
      a = 0;
      break;
    case '"': /* search for end quote */
      while ((c = fgetc(f)) != EOF) {
	if (a >= BUFSIZ) {
	  fprintf(stderr, "%s: %d: Buffer overflow while reading string literal.\n",
		  name, linenum);
	  fclose(f);
	  return;
	}
	switch (c) {
	case '"':
	  goto done_quote;
	case '\r':
	  fprintf(stderr, "%s: %d: Unquoted return character inside string literal.\n",
		  name, linenum);
	  fclose(f);
	  return;
	case '\n':
	  fprintf(stderr, "%s: %d: Unquoted newline inside string literal.\n", name,
		  linenum);
	  fclose(f);
	  return;
	case '\\':
	  c = fgetc(f);
	  if (c == EOF)
	    goto done_quote;
	  if (c == '\n')
	    linenum++;
	  /* FALLTHRU */
	default:
	  buf[a++] = c;
	  continue;
	}
      }
    done_quote:
      continue;
    case '\\': /* literal quote */
      c = fgetc(f);
      if (c == EOF)
	continue;
      if (c == '\n')
	linenum++;
      /* FALLTHRU */
    default:
      buf[a++] = c;
      continue;
    }

    /* this disallows nil-string tokens.  thus "" is always ignored.
     */
    if (buf[0] == '\0')
      continue;

    /* this handles a token depending on the parser state
     */
    switch (state) {
    case parse_void:
      fprintf(stderr, "%s: %d: Syntax error in defaults file\n", name, linenum);
      fclose(f);
      return;

    case parse_state_token:
      if (!strncmp(buf, PathToken, strlen(PathToken)))
	state= parse_path;
      else if (!strncmp(buf, ExtToken, strlen(ExtToken)))
	state= parse_extension;
      else if (!strncmp(buf, FilterToken, strlen(FilterToken)))
	state= parse_filter_name;
      else {
	fprintf(stderr, "%s: %d: Unknown section specifier '%s'.\n",
		name, linenum, buf);
	return;
      }
      break;
    case parse_path:
      if (NumPaths < BUFSIZ - 1)
	Paths[NumPaths++]= expandPath(buf);
      else {
	fprintf(stderr, "%s: %d: Path table overflow\n", name, linenum);
	fclose(f);
	return;
      }
      break;
    case parse_extension:
      if (NumExts < BUFSIZ - 1)
	Exts[NumExts++]= dupString(buf);
      else {
	fprintf(stderr, "%s: %d: Extension table overflow\n", name, linenum);
	fclose(f);
	return;
      }
      break;
    case parse_filter_name: /* name of filter program */
      strcpy(filter_name, buf);
      state= parse_filter_extension;
      break;
    case parse_filter_extension:
      addFilter(buf, filter_name);
      break;
    }
  }
}

void loadPathsAndExts()
{ static int     havepaths= 0;
#ifndef VMS
  struct passwd *pw;
#endif
  char           buf[BUFSIZ];

  if (havepaths)
    return;
  havepaths= 1;

#ifdef VMS
  sprintf(buf, "/sys$login:xloadimage.rc");
#else /* !VMS */
  addFilter(".Z", "uncompress -c"); /* std UNIX uncompress */
  addFilter(".gz", "gzip -cd");     /* same, new extension */
  if (! (pw= (struct passwd *)getpwuid(getuid()))) {
    printf("Can't find your password file entry?!?\n");
    return;
  }
  sprintf(buf, "%s/.xloadimagerc", pw->pw_dir);
#endif /* !VMS */
  if (! access(buf, R_OK)) {
    readPathsAndExts(buf);
    return; /* don't read system file if user has one */
  }
#ifdef SYSPATHFILE
  readPathsAndExts(SYSPATHFILE);
#endif
}

static int fileIsOk(fullname, sbuf)
     char *fullname;
     struct stat *sbuf;
{
  if ((sbuf->st_mode & S_IFMT) == S_IFDIR) /* is a directory */
    return(0);
  return(access(fullname, R_OK)); /* we can read it */
}

/* find an image with paths and extensions from defaults files.  returns
 * -1 if access denied or not found, 0 if ok.
 */

int findImage(name, fullname)
     char *name, *fullname;
{ unsigned int   p, e;
  struct stat    sbuf;

  strcpy(fullname, name);
  if (!strcmp(name, "stdin")) /* stdin is special name */
    return(0);

  /* look for name and name with compress extension
   */
  if (! stat(fullname, &sbuf))
      return(fileIsOk(fullname, &sbuf));
#ifndef NO_COMPRESS
  strcat(fullname, ".Z");
  if (! stat(fullname, &sbuf))
      return(fileIsOk(fullname, &sbuf));
#endif

  for (p= 0; p < NumPaths; p++) {
#ifdef VMS
    sprintf(fullname, "%s%s", Paths[p], name);
#else
    sprintf(fullname, "%s/%s", Paths[p], name);
#endif
    if (! stat(fullname, &sbuf))
      return(fileIsOk(fullname, &sbuf));
#ifndef NO_COMPRESS
    strcat(fullname, ".Z");
    if (! stat(fullname, &sbuf))
#endif
      return(fileIsOk(fullname, &sbuf));
    for (e= 0; e < NumExts; e++) {
#ifdef VMS
      sprintf(fullname, "%s%s%s", Paths[p], name, Exts[e]);
#else
      sprintf(fullname, "%s/%s%s", Paths[p], name, Exts[e]);
#endif
      if (! stat(fullname, &sbuf))
	return(fileIsOk(fullname, &sbuf));
#ifndef NO_COMPRESS
      strcat(fullname, ".Z");
      if (! stat(fullname, &sbuf))
	return(fileIsOk(fullname, &sbuf));
#endif
    }
  }

  for (e= 0; e < NumExts; e++) {
    sprintf(fullname, "%s%s", name, Exts[e]);
    if (! stat(fullname, &sbuf))
      return(fileIsOk(fullname, &sbuf));
#ifndef NO_COMPRESS
    strcat(fullname, ".Z");
    if (! stat(fullname, &sbuf))
      return(fileIsOk(fullname, &sbuf));
#endif
  }
  errno= ENOENT; /* file not found */
  return(-1);
}

/* list images along our path
 */

void listImages()
{ unsigned int a;
  char         buf[BUFSIZ];

  if (!NumPaths) {
    printf("No image path\n");
    return;
  }
  for (a= 0; a < NumPaths; a++) {
    printf("%s:\n", Paths[a]);
    fflush(stdout);
#ifdef VMS
    sprintf(buf, "directory %s", Paths[a]);
#else
    sprintf(buf, "ls %s", Paths[a]);
#endif
    if (system(buf) < 0) {
#ifdef VMS
      perror("directory");
#else
      perror("ls");
#endif
      return;
    }
  }
  return;
}

void showConfiguration()
{ int a;
  struct filter *f;

  if (NumPaths) {
    printf("Image path:");
    for (a= 0; a < NumPaths; a++)
      printf(" %s", Paths[a]);
    printf("\n");
  }
  else
    printf("No image path\n");
  if (NumExts) {
    printf("Image extensions:");
    for (a= 0; a < NumExts; a++)
      printf(" %s", Exts[a]);
    printf("\n");
  }
  else
    printf("No image extensions\n");
  if (Filters) {
    printf("Filters:\n");
    for (f= Filters; f; f= f->next)
      printf(" \"%s\" -> \"%s\"\n", f->extension, f->filter);
    printf("\n");
  }
  else
    printf("No filters\n");
}

char *expandPath(p)
     char *p;
{ char buf1[BUFSIZ], buf2[BUFSIZ];
  int b1, b2, var;
  char *ptr;

  char *getenv();

  buf1[0] = '\0';
  buf2[0] = '\0';
  b1 = 0;
  b2 = 0;
  var = 0;

  while(*p) {
    if(isspace(*p)) break;
#ifndef VMS
    if (*p == '$') var++;
#endif
    else if(*p == '~') {
      buf1[b1] = '\0';
      strcat(buf1, getenv("HOME"));
      b1 = strlen(buf1);
      var = 0;
    }
    else if(*p == '/' || *p == '}') {
      if(var) {
	buf1[b1] = '\0';
	buf2[b2] = '\0';
	strcat(buf1, getenv(buf2));
	b1 = strlen(buf1);
	buf2[0] = '\0';
	b2 = 0;
	var = 0;
      }
      if(*p == '/') {
	buf1[b1] = *p;
	b1++;
      }
    }
    else if(var) {
      if(*p != '{') {
	buf2[b2] = *p;
	b2++;
      }
    }
    else {
      buf1[b1] = *p;
      b1++;
    }
    p++;
  }

  buf1[b1] = '\0';
  
  if((b2 = strlen(buf1)) > 0) {
    ptr = (char *)lmalloc((unsigned) b2+1);
    strcpy(ptr, buf1);
    return(ptr);
  }
  else
    return(NULL);

}
