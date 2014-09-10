/* autoconfig.c:
 *
 * automatic configuration program
 *
 * jim frost 09.05.93
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __STDC__
#include <stdlib.h>
#endif

/* system-specific configuration defaults
 */

#ifdef sgi
/* C compiler has fixed-size parse tree table that overflows on some
 * of the macro translations.  you'd think compiler writers would be
 * smarter, wouldn't you?
 */
#define DEF_CC "cc -Wf,XNh5000"
#endif /* sgi */

/* default versions that work on most systems
 */

#ifndef DEF_CC
#define DEF_CC "cc"
#endif

#ifndef DEF_OPT_FLAGS
#define DEF_OPT_FLAGS "-O"
#endif

#ifndef DEF_INSTALL_DIR
#define DEF_INSTALL_DIR "/usr/local/bin"
#endif

/* list of places to look for system libraries
 */
char *SysLibDirList[] =
{
  "/lib",
  "/usr/lib",
  NULL
};

/* typical system path for finding binaries
 */
char *SysProgPath[] =
{
  "/bin",
  "/usr/bin",
  NULL
};

/* a few miscellaneous names for places people might put graphics libraries
 */
char *MiscIncludeDirList[] = 
{
  "/usr/include",
  "/usr/local/include",
  "/usr/include/graphics",
  "/usr/local/include/graphics",
  ".",  /* why not? */
  NULL
};

char *MiscLibDirList[] =
{
  "/lib",
  "/usr/lib",
  "/usr/local/lib",
  "/usr/graphics/lib",
  "/usr/local/graphics/lib",
  ".",
  NULL
};

struct X11_dir_list { char *inc_dir; char *lib_dir; } X11DirList[] =
{
  { "/usr/include/X11R6", "/usr/lib/X11R6" }, /* HP style; must be first */
  { "/usr/include/X11R5", "/usr/lib/X11R5" }, /* because they have an X11 */
  { "/usr/include/X11R4", "/usr/lib/X11R4" }, /* dir too, but it's empty */
  { "/usr/include", "/usr/lib" },                 /* MIT standard */
  { "/usr/openwin/include", "/usr/openwin/lib" }, /* SunOS */
  { "/usr/X11R6/include", "/usr/X11R6/lib" },
  { "/usr/X11R5/include", "/usr/X11R5/lib" },
  { "/usr/X11R4/include", "/usr/X11R4/lib" },
  { "/usr/local/X11R6/include", "/usr/local/X11R6/lib" },
  { "/usr/local/X11R5/include", "/usr/local/X11R5/lib" },
  { "/usr/local/X11R4/include", "/usr/local/X11R4/lib" },
  { NULL, NULL }
};

struct typical_sys_libs { char *name; char *desc; } TypicalSysLibs[] =
{
  { "socket", "socket support" },
  { "nsl", "network support" },
  { NULL, NULL }
};

char config_style[6]; /* configuration style */
char cflags[4096];    /* flags passed to C compiler */
char libs[4096];      /* extra libraries */
char X11_inc[1024];   /* X11 include path */
char X11_lib[1024];   /* X11 library path */
char X11_name[1024];  /* X11 library name */
char cc[1024];        /* C compiler */
char opt_flags[1024]; /* optimization flags */
char ranlib[1024];    /* ranlib program or /bin/true */
char install_dir[1024]; /* install directory */
char optional_srcs[4096]; /* optional source files */
char optional_libs[4096]; /* optional libraries */

/* this looks to see if there's a file matching a particular pattern.
 * it uses "echo" as a poor-man's glob.
 */
int hasFileNamed(pattern)
     char *pattern;
{
  static char buf[1024];
  FILE *p;
  int i;
  struct stat stat_buf;

  sprintf(buf, "echo %s", pattern);
  p = popen(buf, "r");
  if (p == NULL) { /* bad news */
    perror("popen");
    return(0);
  }
  buf[1023] = '\0';
  if (fgets(buf, 1023, p) == NULL) {
    pclose(p);
    return(0);
  }
  pclose(p);

  if ((buf[0] == ' ') || (buf[0] == '\0'))
    return(0);

  /* in shared-library environments we may get several matches.  just look
   * for the first one to verify existance of any of them.
   */
  for (i = 0; buf[i]; i++) {
    if ((buf[i] == ' ') || (buf[i] == '\n')) {
      buf[i] = '\0';
      break;
    }
  }

  /* technically we could probably get away with avoiding stat()
   */
  if (stat(buf, &stat_buf) < 0)
    return(0);
  return(1);
}

/* this tries to find an installed library.  this uses "echo" as a
 * poor-man's "glob" so we can find shared libraries too.
 */
int hasSysLibrary(name)
     char *name;
{
  char **d;
  char libname[1024];

  for (d = SysLibDirList; *d != NULL; d++) {
    sprintf(libname, "%s/lib%s.*", *d, name);
    if (hasFileNamed(libname))
      return(1);
  }
  return(0);
}

int hasProgram(name)
     char *name;
{
  char **d;
  char pathname[1024];

  for (d = SysProgPath; *d != NULL; d++) {
    sprintf(pathname, "%s/%s", *d, name);
    if (hasFileNamed(pathname))
      return(1);
  }
  return(0);
}

/* functions to find include files and libraries
 */
char *findInclude(name)
{
  char fullname[1024];
  char **d;

  for (d= MiscIncludeDirList; *d; d++) {
    sprintf(fullname, "%s/%s", *d, name);
    if (hasFileNamed(fullname))
      return(*d);
  }
  return(NULL);
}

char *findLibrary(name)
     char *name;
{
  char libname[1024];
  char fullname[1024];
  char **d;

  if (!strncmp(name, "-l", 2))
    sprintf(libname, "lib%s.*", &name[2]); /* skip -l and glob */
  else
    strcpy(libname, X11_name);

  /* ok, go looking
   */
  for (d= MiscLibDirList; *d; d++) {
    sprintf(fullname, "%s/%s", *d, libname);
    if (hasFileNamed(fullname))
      return(*d);
  }
  return(NULL);
}

void addOptionalLibrary(desc, define, libdir, libname, srcname)
     char *desc;    /* human-readable name of library */
     char *define;  /* define that activates support in xloadimage */
     char *libdir;  /* directory where file resides */
     char *libname; /* name of library */
     char *srcname; /* name of include file */
{
  char buf[1024];

  if (!hasFileNamed(libdir)) {
    printf("\nI know how to use the %s library, but it doesn't seem to be\n\
included in this distribution, too bad.\n", libname);
    return;
  }

  printf("\n\
I can include support for the %s format.  Would you like me to do so?\n\
(n=no, default=yes): ", desc);
  gets(buf);
  if (buf[0] == 'n')
    return;
  sprintf(&cflags[strlen(cflags)], " -D%s", define);
  sprintf(&optional_srcs[strlen(optional_srcs)], " %s", srcname);
  sprintf(&optional_libs[strlen(optional_libs)], " %s/%s", libdir, libname);
}

void writeConfigFile()
{ FILE *f;

  f = fopen("Make.conf", "w");
  if (f == NULL) {
    perror("\n*** Could not open Make.conf");
    printf("\
The permissions may be wrong on the file.  Please correct the problem and\n\
try again.\n\n");
    exit(1);
  }
  fprintf(f, "\
# Xloadimage Configuration file.  Normally this file is created by the\n\
# autoconfig program.  If you edit this file, make certain that the\n\
# CONFIG_STYLE line is set to -skip or autoconfig will overwrite your\n\
# changes.\n\
CONFIG_STYLE=%s\n\n", config_style);
  fprintf(f, "\
# C compiler to use, including special flags.\n\
CC=%s\n\n",
	  cc);
  fprintf(f, "\
# Configuration and options flags for C compiler.\n\
CC_FLAGS=%s\n\n",
	  cflags);
  fprintf(f, "\
# Optimization flags for C compiler.\n\
OPT_FLAGS=%s\n\n",
	  opt_flags);
  fprintf(f, "\
# Miscellaneous system libraries.\n\
SYS_LIBS=%s\n\n", libs);
  fprintf(f, "\
# X11 include and library information.\n\
X11_INC_DIR=%s%s\n\
X11_LIB_DIR=%s%s\n\
X11_LIB_NAME=%s\n\n",
	  (X11_inc[0] ? "-I" : ""), X11_inc,
	  (X11_lib[0] ? "-L" : ""), X11_lib,
	  X11_name);
  fprintf(f, "\
INSTALLDIR=%s\n\n", install_dir);
  fprintf(f, "\
# Optional source files, usually integrations for optional loaders.\n\
OPTIONAL_SRCS=%s\n\n",
	  optional_srcs);
  fprintf(f, "\
# Optional libraries, usually loaders.\n\
OPTIONAL_LIBS=%s\n\n",
	  optional_libs);
  fprintf(f, "# Ranlib program, or /bin/true if unnecessary\nRANLIB=%s\n\n",
	  ranlib);
  fprintf(f, "# Other miscellaneous more-or-less standard programs\n");
  fprintf(f, "RM=rm -f\n");
  fprintf(f, "AR=ar cq\n");
  fprintf(f, "CP=cp\n");
  fprintf(f, "MV=mv\n");
  fprintf(f, "LN=ln -s\n");
  fprintf(f, "\n# These fix problems with make cmds that use $SHELL by default\n");
  fprintf(f, "SHELL=/bin/sh\n");
  fprintf(f, "MAKESHELL=/bin/sh\n");
  fclose(f);
}

main(argc, argv)
     int argc;
     char *argv[];
{
  int i;
  int is_sysv = 0;
  int is_bsd = 0;
  int is_debugging = 0;
  int tries;
  char buf[1024];       /* misc input buffer */
  struct typical_sys_libs *tsl;

  /* process command line arguments
   */
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-clean")) {

      /* don't use the DEF_* macros here!  these must be VERY generic!
       */
      strcpy(config_style, "-auto");
      strcpy(cc, "cc");
      strcpy(cflags, "");
      strcpy(opt_flags, "");
      strcpy(libs, "");
      strcpy(X11_inc, "");
      strcpy(X11_lib, "");
      strcpy(X11_name, "-lX11");
      strcpy(ranlib, "ranlib");
      strcpy(install_dir, "/usr/local/bin");
      strcpy(optional_srcs, "");
      strcpy(optional_libs, "");
      writeConfigFile();
      exit(0);
    }
    else if (!strcmp(argv[i], "-bsd")) {
      is_bsd = 1;
      is_sysv = 0;
    }
    else if (!strcmp(argv[i], "-sysv")) {
      is_bsd = 0;
      is_sysv = 1;
    }
    else
      printf("autoconfig: ignoring unknown option '%s'\n", argv[i]);
  }

  printf("\n\
Welcome to the Xloadimage Automatic Configuration Program.\n\n\
I will attempt to determine your machine configuration automatically\n\
and will ask you some questions about preferred compilers and flags.\n\n");

  strcpy(config_style, "-skip");
  strcpy(cflags, "");
  strcpy(libs, "");

  printf("Do you want a debugging version built? (y=yes, default=no): ");
  gets(buf);
  printf("\n");
  if (buf[0] == 'y') {
    printf("I will compile this with debugging information and without optimization.\n");
    is_debugging = 1;
    strcat(cflags, " -DDEBUG");
  }

  if (is_bsd)
    printf("Assuming that this is a BSD-compatible system, as you specified.\n");
  if (is_sysv)
    printf("Assuming that this is a System-V-compatible system, as you specified.\n");

  /* if neither BSD nor SYSV was specified, try to guess which it is.
   * this could be more sophisticated.
   */
  if (!is_bsd && !is_sysv) {
    if (hasFileNamed("/etc/ttytab")) { /* this is a definite BSD-ism */
      printf("This looks like a BSD-compatible system.\n");
      is_bsd = 1;
    }
    else {
      printf("This looks like a System-V-compatible system.\n");
      is_sysv = 1;
    }
  }

  if (is_bsd)
    strcat(cflags, " -DIS_BSD");

  if (is_sysv)
    strcat(cflags, " -DSYSV"); /* needed for <X11/Xos.h> at least */

  /* see if <sys/select.h> exists.  most SysV systems have select()
   * support but require this file to be included in order to use it.
   * if it doesn't exist, look for <poll.h> instead.
   */
  if (hasFileNamed("/usr/include/sys/select.h")) {
    printf("This system has a special header-file for select, so I'll use it.\n");
    strcat(cflags, " -DHAS_SELECT_INCLUDE");
  }
  else if (!is_bsd && hasFileNamed("/usr/include/poll.h")) {
    printf("This system appears to support poll(), so I'll use it.\n");
    strcat(cflags, " -DHAS_POLL");
  }

  /* look for system libraries that are often needed
   */
  for (tsl = TypicalSysLibs; tsl->name != NULL; tsl++) {
    if (hasSysLibrary(tsl->name)) {
      printf("This system appears to have a %s library (-l%s), so I'll use it.\n",
	     tsl->desc, tsl->name);
      strcat(libs, " -l");
      strcat(libs, tsl->name);
    }
  }

  if (hasProgram("ranlib")) {
    printf("This system has ranlib, so I'll use it.\n");
    strcpy(ranlib, "ranlib");
  }
  else
    strcpy(ranlib, "/bin/true");

  /* now for the hard stuff -- we have to find the X11 libraries and
   * include files.
   */
  for (i = 0; X11DirList[i].inc_dir != NULL; i++) {
    char include_path[1024];
    char lib_path[1024];

    sprintf(include_path, "%s/X11", X11DirList[i].inc_dir);
    sprintf(lib_path, "%s/libX11.*", X11DirList[i].lib_dir);
    if (hasFileNamed(include_path) && hasFileNamed(lib_path)) {
      printf("\nI found X11 include files in %s and\n\
the X11 library in %s.\n\
Do you want to use these? (n=no, default=yes): ", include_path, lib_path);
      gets(buf);
      if (buf[0] != 'n')
	break;
    }
  }
  if (X11DirList[i].inc_dir) {
    strcpy(X11_inc, X11DirList[i].inc_dir);
    strcpy(X11_lib, X11DirList[i].lib_dir);
    strcpy(X11_name, "-lX11");
  }
  else {
    strcpy(X11_inc, "");
    printf("\n\
I could not find the X11 include files and/or libraries, so you will need\n\
to specify them for me.  If you don't know where these are you should ask\n\
to your system administrator.\n\n");
    for (tries = 0;; tries++) {
      printf("Where are the include files (eg /usr/local/X11R5/include):\n");
      gets(X11_inc);
      if ((X11_inc[0] == '\0') || (X11_inc[0] == '\n')) {
	printf("\n\
You didn't supply a path so I'm leaving this information out.  You will need\n\
to modify the X11_INC_DIR line in Make.conf for the compilation to succeed,\n\
or type 'make configure' to try configuring the system again.\n");
	strcpy(X11_inc, "");
	break;
      }
      sprintf(buf, "%s/X11", X11_inc);
      if (hasFileNamed(buf))
	break;
      if (tries < 3)
	printf("I cannot find %s.  Please try again.\n", buf);
      else {
	printf("\n\
I'm sorry, but you seem to be having trouble specifying this path.  I'm\n\
leaving it blank and proceeding.  You will probably need to modify the\n\
X11_INC_DIR line in Make.conf for the compilation to succeed.  If you want\n\
to try again, type 'make configure'.\n");
	strcpy(X11_inc, "");
      }
    }

    /* same thing for library path
     */
    for (tries = 0;; tries++) {
      printf("\nWhere are the library files (eg /usr/local/X11R5/lib):\n");
      gets(X11_lib);
      if ((X11_lib[0] == '\0') || (X11_lib[0] == '\n')) {
	printf("\n\
You didn't supply a path so I'm leaving this information out.  You will need\n\
to modify the X11_LIB_DIR line in Make.conf for the compilation to succeed,\n\
or type 'make configure' to try configuring the system again.\n");
	strcpy(X11_lib, "");
	strcpy(X11_name, "-lX11");
	break;
      }
      sprintf(buf, "%s/libX11.*", X11_lib);
      if (hasFileNamed(buf)) {
	strcpy(X11_name, "-lX11");
	break;
      }
      printf("\n\
I can't find %s/libX11.a (or anything that looks like a shared library).\n\
If the library has a different name (eg -lX11R5) type it now, or leave\n\
blank to try again: ", X11_lib);
      gets(X11_name);
      if ((X11_name[0] != '\0') && (X11_name[0] != '\n')) {
	char tmp[1024];
	if (!strncmp(X11_name, "-l", 2))
	  sprintf(tmp, "lib%s.*", &X11_name[2]); /* skip -l and glob */
	else
	  strcpy(tmp, X11_name);
	sprintf(buf, "%s/%s", X11_lib, tmp);
	if (hasFileNamed(buf))
	  break;
	printf("\n\
I couldn't find %s either,\n\
but I'll assume you know what you're doing.  If you have trouble linking\n\
against this library you should edit Make.conf to make the appropriate\n\
changes.\n", buf);
	break;
      }
      if (tries < 3)
	printf("I cannot find %s.  Please try again.\n", buf);
      else {
	printf("\n\
I'm sorry, but you seem to be having trouble specifying this path.  I'm\n\
leaving it blank and proceeding.  You will probably need to modify the\n\
X11_LIB_DIR line in Make.conf for the compilation to succeed.  If you want\n\
to try again, type 'make configure'.\n");
	strcpy(X11_lib, "");
	strcpy(X11_name, "-lX11");
      }
    }
   }

  /* ignore default paths
   */
  if (!strcmp(X11_inc, "/usr/include"))
    strcpy(X11_inc, "");
  
  if (!strcmp(X11_lib, "/usr/lib") ||
      !strcmp(X11_lib, "/lib"))
    strcpy(X11_lib, "");
  
  printf("\n\
The default C compiler on this system is:\n\n\
  %s\n\n\
If you would like to use a different C compiler or add special compilation\n\
flags (other than optimization-related flags), specify its name and flags.\n\
(default %s): ", DEF_CC, DEF_CC);
  gets(cc);
  if ((cc[0] == '\0') || (cc[0] == '\n'))
    strcpy(cc, DEF_CC);
  printf("I will invoke the C compiler using %s.\n", cc);

  if (is_debugging)
    strcpy(opt_flags, "-g");
  else {
    if ((strlen(cc) >= 3) && !strcmp(&cc[strlen(cc) - 3], "gcc")) {
      printf("\n\
You specified gcc.  Significant performance improvements can be gained\n\
by using the optimization flags -fstrength-reduce -finline-functions.\n\
Would you like me to use those flags? (n=no, default=yes): ");
      gets(buf);
      if (buf[0] == 'n')
	goto get_opt_flags; /* ick, but expedient */
      strcpy(opt_flags, "-O -fstrength-reduce -finline-functions");
      printf("\n\
Gcc version 1.37 has some optimizer bugs that can cause problems, but I can\n\
work around them by supplying additional optimization flags.  Do you have\n\
gcc 1.37?  (n=no, default=yes, select default if you don't know): ");
      gets(buf);
      if (buf[0] == 'n')
	strcat(opt_flags, " -fforce-mem -fforce-addr");
    }
    else {
    get_opt_flags:
      printf("\n\
On this system I use the following optimizer flag(s):\n\n\
  %s\n\n\
If you would like to use a different flag or set of flags, enter it now or\n\
leave blank to use the above flag(s): ", DEF_OPT_FLAGS);
      gets(opt_flags);
      if ((opt_flags[0] == '\0') || (opt_flags[0] == '\n'))
	strcpy(opt_flags, DEF_OPT_FLAGS);
    }
    printf("Using optimization flags: %s\n", opt_flags);
  }

  /* ask about the install directory
   */
  printf("\n\
Where would you like the installation installed?  (Installation is NOT\n\
automatic -- you must type 'make install' from an account with the proper\n\
permissions to actually perform the installation.)\n\
(default=%s)? ", DEF_INSTALL_DIR);
  gets(install_dir);
  if ((install_dir[0] == '\0') || (install_dir[0] == '\n'))
    strcpy(install_dir, DEF_INSTALL_DIR);
  printf("Installation directory is %s.\n", install_dir);

  /* now ask about optional loaders we can support if the libraries
   * are present.
   */
  addOptionalLibrary("JPEG", "HAS_JPEG", "jpeg", "libjpeg.a", "jpeg.c");
  addOptionalLibrary("TIFF", "HAS_TIFF", "tiff", "libtiff.a", "tiff.c");

  writeConfigFile();

  printf("\n\
Automatic configuration is complete.  If you need to reconfigure this\n\
installation, type 'make configure'.  If there are problems during\n\
the build process you may need to edit the Make.conf file by hand.\n\n");
  exit(0);
}
