/* niff.h:
 *
 * header information for xloadimage's native image file format (NIFF),
 * version 1.
 *
 * jimf 10.29.91
 *
 * this is in the public domain
 */

#define NIFF_MAGIC 0x4e494646 /* "NIFF" */
#define NIFF_VERSION 1        /* this version */

/* types of image data
 */
#define NIFF_BITMAP 1
#define NIFF_RGB    2
#define NIFF_TRUE   3

/* colormap entry
 */
struct niff_cmap {
    char red[2];
    char green[2];
    char blue[2];
};

struct niff_header {
    char magic[4];     /* NIFF magic number */
    char version[4];   /* NIFF version */
    char width[4];     /* image dimensions */
    char height[4];
    char depth[4];     /* image depth in bits (true depth, not rounded) */
    char type;         /* image type */
    char cmap_size[4]; /* size of colormap; zero NIFF_TRUE */
    char title_len[4]; /* length of image title */
    /* title */
    /* cmap */
    /* image data */
};

/* the image title follows the header unless title_len is zero.
 *
 * the image colormap follows the title unless the image is NIFF_TRUE
 * (there must always be at least one cmap entry for NIFF_BITMAP and
 * NIFF_RGB images).
 *
 * the image data follows the cmap data.
 *
 * NIFF_BITMAP data is padded to 8-bits, the highest-order bit is the
 * lowest bitmap pixel.
 *
 * NIFF_RGB data is padded to 8-bits per pixel, highest-order byte first.
 * the number of bytes per pixel is determined by cmap_size (calculate
 * bits-per-pixel using cmap_size and round up to an 8-bit boundary).
 *
 * NIFF_TRUE data is always 3-bytes per pixel - red followed by green
 * followed by blue, 8 bits each. depth must be 24 and cmap_size must be
 * zero.
 */
