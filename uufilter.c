/* uufilter.c:
 *
 * "smart" uudecode function that skips junk lines and outputs data to
 * stdout rather than a filename.
 *
 * jim frost 10.21.93
 *
 * 10.27.93 fixed a bug where short uuencoded lines were stripped out
 *          due to a bad "short length" calculation.  added -v flag at
 *          the same time.
 */

#include <stdio.h>

main(argc, argv)
     int argc;
     char **argv;
{
  FILE *inf, *outf;
  char *infilename = NULL;
  char *outfilename = NULL;
  int dump_to_file = 1; /* true if we're outputting to a file */
  int stringent = 1; /* true if we're really stringent about data lines */
  int verbose = 0; /* true if we're babbling about what we're skipping */
  char buf[1024];
  int i, len;
  int phase = 0;
  unsigned char outchar;

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-s")) /* output to stdout */
      dump_to_file = 0;
    else if (!strcmp(argv[i], "-l")) /* lenient mode */
      stringent = 0;
    else if (!strcmp(argv[i], "-f")) { /* output to named file */
      dump_to_file = 1;
      outfilename = argv[++i];
    }
    else if (!strcmp(argv[i], "-v")) /* verbose output */
      verbose = 1;
    else {
      infilename = argv[i];
      break;
    }
  }
  if (infilename == NULL) {
    inf = stdin;
    infilename = "stdin";
  }
  else {
    inf = fopen(infilename, "r");
    if (inf == NULL) {
      perror(infilename);
      exit(1);
    }
  }

  /* scan for "begin" line
   */
  while (fgets(buf, 1024, inf) != NULL) {
    if (!strncmp(buf, "begin ", 6))
      break;
    if (verbose)
      fprintf(stderr, "Ignoring header line: %s\n", buf);
  }
  if (feof(inf)) {
    fprintf(stderr, "No 'begin' line, sorry.\n", infilename);
    exit(1);
  }

  if (dump_to_file) {
    int mode; /* ignored */
    char tmp[1024];

    if (!outfilename) {
      if (sscanf(buf, "begin %o %s", &mode, tmp) != 2) {
	fprintf(stderr, "%s: Bad format for 'begin' line (can't dump to file)\n", infilename);
	exit(1);
      }
      outfilename = tmp;
    }
    fprintf(stderr, "Decoding into file %s\n", outfilename);
    outf = fopen(outfilename, "w");
    if (outf == NULL) {
      perror(outfilename);
      exit(1);
    }
  }
  else
    outf = stdout;

  for (;;) {
    char *bp;

    if (fgets(buf, 1024, inf) == NULL) {
      fprintf(stderr, "%s: Missing 'end' line.\n", infilename);
      break;
    }
	
    if (!strncmp(buf, "end", 3))
      break; /* end of data */

    /* strip off trailing characters
     */
    len = strlen(buf) - 1;
    while ((buf[len] == '\n') || (buf[len] == '\r'))
      buf[len--] = '\0';

    /* character conversion and "obvious" filtering
     */
    for (; len >= 0; len--) {
      if ((buf[len] >= 'a') || (buf[len] < ' '))
	break; /* garbage character seen */

      if (buf[len] == '`')
	buf[len] = ' ';
    }
    if (len >= 0) {
      if (verbose)
	fprintf(stderr, "Ignoring garbage line: %s\n", buf);
      continue; /* line had garbage, ignore it */
    }

    if (buf[0] == '\0')
      continue; /* line is blank, ignore it */

    len = buf[0] - ' ';

    /* ok, see if the line length looks sane.
     */
    {
      int line_len = strlen(buf) - 1;
      int allow_len;

      /* if the line is shorter than the absolute minimum
       * allowable then we're sure this is not a valid line.
       */
      allow_len = ((len * 4) / 3) + ((len * 4) % 3);
      if (line_len < allow_len) {
	if (verbose)
	  fprintf(stderr, "Ignoring short line: %s\n", buf);
	continue;
      }

      /* if we're being really stringent about the lines we allow
       * this filters out any lines that are too long, too.  note
       * that this is still fairly lenient; it allows uuencode
       * programs to pad the line length out to a multiple of
       * 3 bytes.
       */
      if (stringent) {
	allow_len = ((len + 2) * 4) / 3;
	if (line_len > allow_len) {
	  if (verbose)
	    fprintf(stderr, "Ignoring long line: %s\n", buf);
	  continue;
	}
      }
    }

    /* ok, we have some characters -- process them!
     */
    bp = &buf[1];
    phase = 0;
    while (len > 0) {
      unsigned char c;

      c = (unsigned char)*(bp++) - ' ';
      switch (phase) {
      case 0: /* 1st 6 bits */
	outchar = (c << 2);
	break;
      case 1: /* last 2 bits, 1st 4 bits */
	outchar |= (c >> 4);
	fputc(outchar, outf);
	len--;
	outchar = (c & 0xf) << 4;
	break;
      case 2: /* last 4 bits, 1st 2 bits */
	outchar |= (c >> 2);
	fputc(outchar, outf);
	len--;
	outchar = (c & 0x3) << 6;
	break;
      case 3: /* last 6 bits */
	outchar |= c;
	fputc(outchar, outf);
	len--;
	phase = 0;
	continue;
      }
      phase++;
    }
  }
  /* flush output
   */
  if (outf == stdout)
    fflush(outf);
  else
    fclose(outf);

  /* suck up the rest of the input file.  this avoids "broken pipe"
   * errors if reading from stdin.
   */
  if (inf == stdin) {
    while (fgets(buf, 1024, inf) != NULL)
      if (verbose)
	fprintf(stderr, "Ignoring trailing line: %s\n", buf);
  }
  else
    fclose(inf);

  exit(0);
}
