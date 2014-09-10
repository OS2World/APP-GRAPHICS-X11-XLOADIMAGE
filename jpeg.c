/*
 * jpeg - This file provides interface routines to connect xloadimage to
 * the jpeg decoding routines taken from the Independent JPEG Group's
 * free JPEG software.  See jpeg.README for more information.
 *
 * This code is based on example.c from the IJG v4 distribution.
 */

#include "image.h"	/* xloadimage declarations */
/*#  include "jpeg.conf.h"*/  /* definitions used in jpeg directory */
#include "jpeg/jinclude.h" /* IJG declarations */
#include <setjmp.h>	/* need setjmp/longjmp */

/* Error-catching routines */

static char *filename;
static unsigned int verbose;
static unsigned int identify;
static jmp_buf setjmp_buffer;	/* for return to caller */
static external_methods_ptr emethods; /* needed for access to message_parm */

static void
trace_message (msgtext)
     char *msgtext;
{
  fprintf(stderr, "jpegLoad: %s - ", filename);
  fprintf(stderr, msgtext,
	  emethods->message_parm[0], emethods->message_parm[1],
	  emethods->message_parm[2], emethods->message_parm[3],
	  emethods->message_parm[4], emethods->message_parm[5],
	  emethods->message_parm[6], emethods->message_parm[7]);
  fprintf(stderr, "\n");	/* there is no \n in the format string! */
}

static void
error_exit (msgtext)
     char *msgtext;
{
  trace_message(msgtext);	/* report the error message */
  (*emethods->free_all) ();	/* clean up memory allocation & temp files */
  longjmp(setjmp_buffer, 1);	/* return control to outer routine */
}


/* Output-acceptance routines */

static Image *image;		/* xloadimage image being returned */
static int rows_put;		/* Number of rows copied to image */


static void
output_init (cinfo)
     decompress_info_ptr cinfo;
/* Initialize for output */
{
  int i;

  if (cinfo->out_color_space == CS_GRAYSCALE) {
    image = newRGBImage(cinfo->image_width,cinfo->image_height,8);
    image->title = dupString(filename);
    /* set a linear map */
    for(i=0;i<256;i++) {
      *(image->rgb.red + i) = 
	*(image->rgb.green + i) = 
	  *(image->rgb.blue + i) = i<<8;
    }
    image->rgb.used = 256;
  } else if (cinfo->out_color_space == CS_RGB) {
    image = newTrueImage(cinfo->image_width,cinfo->image_height);
    image->title = dupString(filename);
  } else {
    image = NULL;
    ERREXIT(cinfo->emethods, "Cannot cope with JPEG image colorspace");
  }
  rows_put = 0;
}


static void
put_color_map (cinfo, num_colors, colormap)
     decompress_info_ptr cinfo;
     int num_colors;
     JSAMPARRAY colormap;
/* Write the color map -- should not be called */
{
  fprintf(stderr, "put_color_map called: there is a bug here somewhere!\n");
}


static void
put_pixel_rows (cinfo, num_rows, pixel_data)
     decompress_info_ptr cinfo;
     int num_rows;
     JSAMPIMAGE pixel_data;
/* Write some rows of output data */
{
  register unsigned char *bufp;
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  long width = cinfo->image_width;
  int row;
  
  if (cinfo->out_color_space == CS_GRAYSCALE) {
    bufp = image->data + rows_put * width;
    /* Assume JSAMPLE == chars */
    for (row = 0; row < num_rows; row++) {
      bcopy(pixel_data[0][row],bufp,width);
      bufp += width;
    }
  } else {
    bufp = image->data + rows_put * width * 3;
    for (row = 0; row < num_rows; row++) {
      ptr0 = pixel_data[0][row];
      ptr1 = pixel_data[1][row];
      ptr2 = pixel_data[2][row];
      for (col = width; col > 0; col--) {
	*bufp++ = *ptr0++;
	*bufp++ = *ptr1++;
	*bufp++ = *ptr2++;
      }
    }
  }
  rows_put += num_rows;
}


static void
output_term (cinfo)
     decompress_info_ptr cinfo;
/* Finish up at the end of the output */
{
  /* No work here */
}


/* Input-file-reading routine */


static ZFILE * input_file;	/* tells input routine where to read JPEG */


static int
read_jpeg_data (cinfo)
     decompress_info_ptr cinfo;
{
  cinfo->next_input_byte = cinfo->input_buffer + MIN_UNGET;

  cinfo->bytes_in_buffer = zread(input_file,
				 (byte *)cinfo->next_input_byte,
				 JPEG_BUF_SIZE);
  
  if (cinfo->bytes_in_buffer <= 0) {
    WARNMS(cinfo->emethods, "Premature EOF in JPEG file");
    cinfo->next_input_byte[0] = (char) 0xFF;
    cinfo->next_input_byte[1] = (char) 0xD9; /* EOI marker */
    cinfo->bytes_in_buffer = 2;
  }

  return JGETC(cinfo);
}


/*  Required control-hook routine */


static void
d_ui_method_selection (cinfo)
     decompress_info_ptr cinfo;
{
  /* if grayscale input, force grayscale output; */
  /* else leave the output colorspace as set by main routine. */
  if (cinfo->jpeg_color_space == CS_GRAYSCALE)
    cinfo->out_color_space = CS_GRAYSCALE;

  /* Create display of image parameters */
  if (verbose) {
    printf("%s is a %dx%d JPEG image, color space ", filename,
	   cinfo->image_width, cinfo->image_height);
    switch (cinfo->jpeg_color_space) {
    case CS_UNKNOWN:
      printf("Unknown");
      break;
    case CS_GRAYSCALE:
      printf("Grayscale");
      break;
    case CS_RGB:
      printf("RGB");
      break;
    case CS_YCbCr:
      printf("YCbCr");
      break;
    case CS_YIQ:
      printf("YIQ");
      break;
    case CS_CMYK:
      printf("CMYK");
      break;
    }
    printf(", %d comp%s,", cinfo->num_components,
	   cinfo->num_components ? "s." : ".");
    if (cinfo->arith_code)
      printf(" Arithmetic coding\n");
    else
      printf(" Huffman coding\n");
  }

  /* Turn off caching beyond this point of the file */
  znocache(input_file);

  /* If we only wanted to identify the image, abort now */
  if (identify) {
    (*emethods->free_all) ();	/* clean up memory allocation & temp files */
    longjmp(setjmp_buffer, 10);	/* return control with success code */
  }

  /* select output routines */
  cinfo->methods->output_init = output_init;
  cinfo->methods->put_color_map = put_color_map;
  cinfo->methods->put_pixel_rows = put_pixel_rows;
  cinfo->methods->output_term = output_term;
}


/* Main control routine for loading */


Image *
jpegLoad (fullname, name, vbose)
     char *fullname, *name;
     unsigned int vbose;
{
  struct Decompress_info_struct cinfo;
  struct Decompress_methods_struct dc_methods;
  struct External_methods_struct e_methods;

  input_file = zopen(fullname);	/* Open the input file */
  if (input_file == NULL)
    return NULL;

  /* Quick check to see if file starts with JPEG SOI marker */
  if (zgetc(input_file) != 0xFF || zgetc(input_file) != 0xD8) {
    zclose(input_file);
    return NULL;
  }

  filename = name;		/* copy parms to static vars */
  verbose = vbose;
  identify = 0;

  image = NULL;			/* in case we fail before creating image */

  cinfo.methods = &dc_methods;	/* links to method structs */
  cinfo.emethods = &e_methods;
  emethods = &e_methods;	/* save struct addr for possible access */
  e_methods.error_exit = error_exit; /* supply error-exit routine */
  e_methods.trace_message = trace_message; /* supply trace-message routine */
  e_methods.trace_level = 0;	/* default = no tracing */
  e_methods.num_warnings = 0;	/* no warnings emitted yet */
  e_methods.first_warning_level = 0; /* display first corrupt-data warning */
  e_methods.more_warning_level = 3; /* but suppress additional ones */

  /* prepare setjmp context for possible exit from error_exit */
  if (setjmp(setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error. */
    /* Return as much of the image as we could get. */
    zclose(input_file);
    return image;
  }

  jselmemmgr(&e_methods);	/* select std memory allocation routines */

  /* Set up default decompression parameters. */
  j_d_defaults(&cinfo, TRUE);

  /* Override default methods */
  dc_methods.d_ui_method_selection = d_ui_method_selection;
  dc_methods.read_jpeg_data = read_jpeg_data;

  /* Insert fake SOI into the input buffer --- needed cause we read it above */
  cinfo.next_input_byte = cinfo.input_buffer + MIN_UNGET;
  cinfo.next_input_byte[0] = (char) 0xFF;
  cinfo.next_input_byte[1] = (char) 0xD8; /* SOI marker */
  cinfo.bytes_in_buffer = 2;

  /* Set up to read a JFIF or baseline-JPEG file. */
  /* This is the only JPEG file format currently supported. */
  jselrjfif(&cinfo);

  /* Here we go! */
  jpeg_decompress(&cinfo);

  zclose(input_file);		/* Done, close the input file */

  return image;
}


/* Main control routine for identifying JPEG without loading */


int
jpegIdent (fullname, name)
     char *fullname, *name;
{
  struct Decompress_info_struct cinfo;
  struct Decompress_methods_struct dc_methods;
  struct External_methods_struct e_methods;

  input_file = zopen(fullname);	/* Open the input file */
  if (input_file == NULL)
    return 0;

  /* Quick check to see if file starts with JPEG SOI marker */
  if (zgetc(input_file) != 0xFF || zgetc(input_file) != 0xD8) {
    zclose(input_file);
    return 0;
  }

  /* We want to find and display the image dimensions, and also
   * verify that the header markers are not corrupt.  To do this,
   * we fire up the JPEG decoder as normal, but when d_ui_method_selection
   * is called, we abort the process by longjmp'ing back here.
   * This works nicely since the headers are all read at that point.
   */

  filename = name;		/* copy parms to static vars */
  verbose = 1;
  identify = 1;

  cinfo.methods = &dc_methods;	/* links to method structs */
  cinfo.emethods = &e_methods;
  emethods = &e_methods;	/* save struct addr for possible access */
  e_methods.error_exit = error_exit; /* supply error-exit routine */
  e_methods.trace_message = trace_message; /* supply trace-message routine */
  e_methods.trace_level = 0;	/* default = no tracing */
  e_methods.num_warnings = 0;	/* no warnings emitted yet */
  e_methods.first_warning_level = 0; /* display first corrupt-data warning */
  e_methods.more_warning_level = 3; /* but suppress additional ones */

  /* prepare setjmp context for expected exit via longjmp */
  switch (setjmp(setjmp_buffer)) {
  case 0:
    /* First time thru, keep going */
    break;
  case 10:
    /* Successful exit from d_ui_method_selection; return A-OK */
    zclose(input_file);
    return 1;
  default:
    /* If we get here, the JPEG code has signaled an error. */
    /* Return 0 since error in the headers means the image is unloadable. */
    zclose(input_file);
    return 0;
  }

  jselmemmgr(&e_methods);	/* select std memory allocation routines */

  /* Set up default decompression parameters. */
  j_d_defaults(&cinfo, TRUE);

  /* Override default methods */
  dc_methods.d_ui_method_selection = d_ui_method_selection;
  dc_methods.read_jpeg_data = read_jpeg_data;

  /* Insert fake SOI into the input buffer --- needed cause we read it above */
  cinfo.next_input_byte = cinfo.input_buffer + MIN_UNGET;
  cinfo.next_input_byte[0] = (char) 0xFF;
  cinfo.next_input_byte[1] = (char) 0xD8; /* SOI marker */
  cinfo.bytes_in_buffer = 2;

  /* Set up to read a JFIF or baseline-JPEG file. */
  /* This is the only JPEG file format currently supported. */
  jselrjfif(&cinfo);

  /* Here we go! */
  jpeg_decompress(&cinfo);

  /* Don't expect to get here since d_ui_method_selection should do longjmp */

  zclose(input_file);
  return 0;
}

/* information necessary to extract image data
 */
static struct {
  Image *image;
  byte *current_row;
  unsigned int bytes_per_row;
} ReadInfo;

static void input_init(cinfo)
compress_info_ptr cinfo;
{
  /* this is done in jpegDump()
   */
}

static void input_term(cinfo)
compress_info_ptr cinfo;
{
  /* there is no shutdown necessary
   */
}

/* this reads a single raster line
 */
static void read_row(cinfo, pixel_rows)
     compress_info_ptr cinfo;
     JSAMPARRAY pixel_rows;
{
  register int x;
  register int pixlen;
  register byte *src_row_ptr;
  register byte *dest_red_ptr;
  register byte *dest_green_ptr;
  register byte *dest_blue_ptr;
  register Pixel pixval;
  register byte mask;

  switch (ReadInfo.image->type) {
  case IBITMAP:
    mask = 0x80;
    src_row_ptr = ReadInfo.current_row;
    dest_red_ptr = (byte *)pixel_rows[0];
    for (x = 0; x < cinfo->image_width; x++) {
      pixval = ((*src_row_ptr & mask) > 0 ? 1 : 0);

      /* we use the "red" color value under the assumption that they
       * are all equal.  that can be wrong if the user used -foreground
       * or -background.  I don't care right now.
       */
      *(dest_red_ptr++) = ReadInfo.image->rgb.red[pixval] >> 8;
      mask >>= 1;
      if (mask == 0) {
	mask = 0x80;
	src_row_ptr++;
      }
    }
    break;

  case IRGB:
    /* this expands the pixel value into its components
     */
    pixlen = ReadInfo.image->pixlen;
    src_row_ptr = ReadInfo.current_row;
    dest_red_ptr = (byte *)pixel_rows[0];
    dest_green_ptr = (byte *)pixel_rows[1];
    dest_blue_ptr = (byte *)pixel_rows[2];
    for (x = 0; x < cinfo->image_width; x++) {
      pixval = memToVal(src_row_ptr, pixlen);
      *(dest_red_ptr++) = ReadInfo.image->rgb.red[pixval] >> 8;
      *(dest_green_ptr++) = ReadInfo.image->rgb.green[pixval] >> 8;
      *(dest_blue_ptr++) = ReadInfo.image->rgb.blue[pixval] >> 8;
      src_row_ptr += pixlen;
    }
    break;

  case ITRUE:
    src_row_ptr = ReadInfo.current_row;
    dest_red_ptr = (byte *)pixel_rows[0];
    dest_green_ptr = (byte *)pixel_rows[1];
    dest_blue_ptr = (byte *)pixel_rows[2];
    for (x = 0; x < cinfo->image_width; x++) {
      *(dest_red_ptr++) = *(src_row_ptr++);
      *(dest_green_ptr++) = *(src_row_ptr++);
      *(dest_blue_ptr++) = *(src_row_ptr++);
    }
    break;
  }
  ReadInfo.current_row += ReadInfo.bytes_per_row;
}

/*
 * This routine gets control after the input file header has been read.
 * It must determine what output JPEG file format is to be written,
 * and make any other compression parameter changes that are desirable.
 */

static void
c_ui_method_selection (cinfo)
     compress_info_ptr cinfo;
{
  /* If the input is gray scale, generate a monochrome JPEG file. */
  if (cinfo->in_color_space == CS_GRAYSCALE)
    j_monochrome_default(cinfo);
  jselwjfif(cinfo);
}

/* parse options passed to jpegDump
 */
static void parseOptions(cinfo, options, verbose)
     compress_info_ptr cinfo;
     char *options;
     int verbose;
{
  char *name, *value;

  /* (Re-)initialize the system-dependent error and memory managers. */
  jselerror(cinfo->emethods);	/* error/trace message routines */
  jselmemmgr(cinfo->emethods);	/* memory allocation routines */
  cinfo->methods->c_ui_method_selection = c_ui_method_selection;

  /* Set up default JPEG parameters. */
  /* Note that default -quality level here need not, and does not,
   * match the default scaling for an explicit -qtables argument.
   */
  j_c_defaults(cinfo, 75, FALSE); /* default quality level = 75 */

  while (getNextTypeOption(&options, &name, &value) > 0) {
    if (!strncmp("arithmetic", name, strlen(name))) {
      /* Use arithmetic coding. */
#ifdef C_ARITH_CODING_SUPPORTED
      if (verbose)
	printf("  Using arithmetic coding.\n");
      cinfo->arith_code = TRUE;
#else
      fprintf(stderr, "jpegDump: sorry, arithmetic coding not supported\n");
    }
#endif
    else if (!strncmp("grayscale", name, strlen(name)) ||
	     !strncmp("greyscale", name, strlen(name)) ||
	     !strncmp("monochrome", name, strlen(name))) {
      /* Force a monochrome JPEG file to be generated. */
      if (verbose)
	printf("  Creating a grayscale/monochrome file.\n");
      j_monochrome_default(cinfo);
    }
    else if (!strncmp("nointerleave", name, strlen(name))) {
      /* Create noninterleaved file. */
#ifdef C_MULTISCAN_FILES_SUPPORTED
      if (verbose)
	printf("  Creating a noninterleaved file.\n");
      cinfo->interleave = FALSE;
#else
      fprintf(stderr, "jpegDump: sorry, multiple-scan support was not compiled\n");
#endif
    }
    else if (!strncmp("optimize", name, strlen(name)) ||
	     !strncmp("optimise", name, strlen(name))) {
      /* Enable entropy parm optimization. */
#ifdef ENTROPY_OPT_SUPPORTED
      if (verbose)
	printf("  Optimizing entropy.\n");
      cinfo->optimize_coding = TRUE;
#else
      fprintf(stderr, "jpegDump: sorry, entropy optimization was not compiled\n");
#endif
    }
    else if (!strncmp("quality", name, strlen(name))) {
      /* Quality factor (quantization table scaling factor). */
      int val;

      if (!value || (sscanf(value, "%d", &val) != 1))
	fprintf(stderr, "jpegDump: quality: Invalid quality factor specified.\n");
      else {
	/* Set quantization tables (will be overridden if -qtables also given).
	 * Note: we make force_baseline FALSE.
	 * This means non-baseline JPEG files can be created with low Q values.
	 * To ensure only baseline files are generated, pass TRUE instead.
	 */
	if (verbose)
	  printf("  Using a quality factor of %d.\n", val);
	j_set_quality(cinfo, val, FALSE);
#if 0
	/* Change scale factor in case -qtables is present. */
	q_scale_factor = j_quality_scaling(val);
#endif
      }
    }
#if 0
    else if (!strncmp("qtables", name, strlen(name))) {
      /* Quantization tables fetched from file. */
      if (!value || !*value)
	fprintf(stderr, "jpegDump: No file specified for quantization tables.\n");
      else {
	if (verbose)
	  printf("  Using quantization tables loaded from %s.\n", qtablefile);
	qtablefile = value;
      }
      /* we postpone actually reading the file in case -quality comes later */
    }
#endif
    else if (!strncmp("restart", name, strlen(name))) {
      /* Restart interval in MCU rows (or in MCUs with 'b'). */
      long lval;
      char ch = 'x';

      if (!value || (sscanf(value, "%ld%c", &lval, &ch) < 1) ||
	  (lval < 0) || (lval > 65535L)) {
	fprintf(stderr, "jpegDump: restart: Invalid restart interval specified.\n");
	continue;
      }
      if (verbose)
	printf("  Using a restart interval of %s.\n", value);
      if (ch == 'b' || ch == 'B')
	cinfo->restart_interval = (UINT16) lval;
      else
	cinfo->restart_in_rows = (int) lval;
    }
#if 0
    else if (!strncmp("sample", name, strlen(name))) {
      /* Set sampling factors. */
      if (!value)
	fprintf(stderr, "jpegDump: sample: No sampling factors specified.\n");
      if (verbose)
	printf("  Using sampling factors of %s.\n", value);
      set_sample_factors(cinfo, value);
    }
#endif
    else if (!strncmp("smooth", name, strlen(name))) {
      /* Set input smoothing factor. */
      int val;

      if (!value || (sscanf(value, "%d", &val) != 1) ||
	  (val < 0) || (val > 100))
	fprintf(stderr, "jpegDump: smooth: Invalid smoothing factor specified.\n");
      else {
	if (verbose)
	  printf("  Using a smoothing factor of %d.\n", val);
	cinfo->smoothing_factor = val;
      }
    }
    else
      fprintf(stderr, "jpegDump: Unknown option '%s'.\n", name);
  }
}

void jpegDump(image, options, file, verbose)
    Image *image;
    char *options;
    char *file;
{
  struct Compress_info_struct cinfo;
  struct Compress_methods_struct c_methods;
  struct External_methods_struct e_methods;

  if (verbose)
    printf("Dumping JFIF-style JPEG image to %s.\n", file);

  /* Set up links to method structures. */
  cinfo.methods = &c_methods;
  cinfo.emethods = &e_methods;

  /* set up "input methods" that handle "reading" from our image file
   */
  cinfo.methods->input_init = input_init;
  cinfo.methods->input_term = input_term;
  cinfo.methods->get_input_row = read_row;

  /* set up output file; there is no input file
   */
  cinfo.input_file = NULL;
  cinfo.output_file = fopen(file, "w");
  if (cinfo.output_file == NULL) {
    perror(file);
    return;
  }

  ReadInfo.image = image;
  ReadInfo.current_row = image->data;

  /* parse the options the user gave us
   */
  parseOptions(&cinfo, options, verbose);

  /* set up image information
   */
  cinfo.image_width = image->width;
  cinfo.image_height = image->height;

  switch (image->type) {
  case IBITMAP:
    ReadInfo.bytes_per_row = (image->width / 8) + (image->width % 8 ? 1 : 0);
    cinfo.input_components = 1;
    cinfo.in_color_space = CS_GRAYSCALE;
    cinfo.data_precision = 8;
    break;
  case IRGB:
    ReadInfo.bytes_per_row = image->width * image->pixlen;
    cinfo.input_components = 3;
    cinfo.in_color_space = CS_RGB;
    cinfo.data_precision = 8;
    break;
  case ITRUE:
    ReadInfo.bytes_per_row = image->width * image->pixlen;
    cinfo.input_components = 3;
    cinfo.in_color_space = CS_RGB;
    cinfo.data_precision = 8;
    break;
  }

  /* compress, baby
   */
  jpeg_compress(&cinfo);

  fclose(cinfo.output_file);
  return;
}
