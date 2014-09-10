/* jimf@saber.com, 05.06.91
 *
 * fairly major modifications to get this to work well in the xloadimage
 * view of the world.  rather than a standalone program, this is now designed
 * to be run as a subroutine.  a lot of comments on how to build and use
 * this on different kinds of hosts have been removed.  removed or rewrote
 * all the host-portability stuff.  lobotomized its ability to dump any but
 * VICAR format.
 */
/* datri@convex.com, 4-14-91
   This file is taken almost unchanged from the NASA CD-ROMS of the Voyager
   images.  I can find no copyright anywhere near this code.
   All I've done is added - as a special case for the output
   filename to go to stdout, and made informational messages go to stderr
   instead of stdout, removing some of the silly ones.  I've also corrected
   some of the places where fools assume that VAX and VMS are synonymous */

/********************************************************************/
/*  Voyager Image Decompression Program - C Version for PC, VAX,    */
/*  UNIX and Macintosh systems.                                     */
/*                                                                  */
/*  Decompresses images using Kris Becker's subroutine DECOMP.C     */
/*  which is included in this program in a shortened version.       */
/*                                                                  */
/*  Reads a variable length compressed VOYAGER image and outputs a  */
/*  fixed length uncompressed image file in PDS format with         */
/*  labels, image histogram, engineering table and 800 lines of     */
/*  836 bytes (800 samples, 36 engineering bytes); or an 800 by     */
/*  800 array with FITS, VICAR or no labels.  If used on a non-     */
/*  byte-swapped machine the image histogram is un-swapped.         */
/*                                                                  */
/********************************************************************/
/* HIST                                                             */
/*  AUG89 Added code to get command line arguments for filenames    */
/*  and output format; routines to free memory used by the Huffman  */
/*  tree); fixed the SFDU label output length; and modified the     */
/*  I/O routines so that the open for Host type 2 uses binary I/O.  */
/*  JUN89 Fixed READVAR, to get length on 16-bit unswapped hosts.   */
/*  JUL88 C driver to decompress standard Voyager Compressed images */
/*  by Mike Martin 1989/06/10                                       */
/*                                                                  */
/*  Inputs   - Input file to be decompressed.                       */
/*                                                                  */
/*  Outputs  - Output file containing decompressed image.           */
/*                                                                  */
/********************************************************************/

#include "image.h"
#include "pds.h"

#define RECORD_BYTES        836

typedef struct leaf {
  struct leaf *right;
  short int dn;
  struct leaf *left;
} NODE;

NODE        *tree;             /* huffman tree pointer */
static long  hist[511];        /* histogram for huffman code */
static int   Decompressing= 0; /* flag for pdsRead() */

static NODE *new_node();
static long int free_node();

NODE *huff_tree(hist)
/****************************************************************************
*_TITLE huff_tree - constructs the Huffman tree; returns pointer to root    *
*_ARGS  TYPE          NAME        I/O   DESCRIPTION                         */
        long int     *hist;     /* I    First difference histogram          */

{
  /*  Local variables used */
  long int freq_list[512];      /* Histogram frequency list */
  NODE **node_list;             /* DN pointer array list */

  register long int *fp;        /* Frequency list pointer */
  register NODE **np;           /* Node list pointer */

  register long int num_freq;   /* Number non-zero frequencies in histogram */
  long int sum;                 /* Sum of all frequencies */

  register short int num_nodes; /* Counter for DN initialization */
  register short int cnt;       /* Miscellaneous counter */

  short int znull = -1;         /* Null node value */

  register NODE *temp;          /* Temporary node pointer */

  /* Functions called */
  void sort_freq();

  /***************************************************************************
    Allocate the array of nodes from memory and initialize these with numbers
    corresponding with the frequency list.  There are only 511 possible
    permutations of first difference histograms.  There are 512 allocated
    here to adhere to the FORTRAN version.
   ****************************************************************************/

  fp = freq_list;
  node_list = (NODE **) lmalloc(sizeof(temp)*512);
  if (node_list == NULL) {
    fprintf(stderr,"\nOut of memory in huff_tree!\n");
    exit(1);
  }
  np = node_list;

  for (num_nodes=1, cnt=512 ; cnt-- ; num_nodes++) {
/**************************************************************************
    The following code has been added to standardize the VAX byte order
    for the "long int" type.  This code is intended to make the routine
    as machine independant as possible.
***************************************************************************/
    unsigned char *cp = (unsigned char *) hist++;
    unsigned long int j;
    short int i;
    for (i=4 ; --i >= 0 ; j = (j << 8) | *(cp+i))
      ;

    /* Now make the assignment */
    *fp++ = j;
    temp = new_node(num_nodes);
    *np++ = temp;
  }

  (*--fp) = 0;         /* Ensure the last element is zeroed out.  */

/***************************************************************************
  Now, sort the frequency list and eliminate all frequencies of zero.
****************************************************************************/

  num_freq = 512;
  sort_freq(freq_list,node_list,num_freq);

  fp = freq_list;
  np = node_list;

  for (num_freq=512 ; (*fp) == 0 && (num_freq) ; fp++, np++, num_freq--)
    /* EMPTY */
    ;

/***************************************************************************
  Now create the tree.  Note that if there is only one difference value,
  it is returned as the root.  On each interation, a new node is created
  and the least frequently occurring difference is assigned to the right
  pointer and the next least frequency to the left pointer.  The node
  assigned to the left pointer now becomes the combination of the two
  nodes and it's frequency is the sum of the two combining nodes.
****************************************************************************/

  for (temp=(*np) ; (num_freq--) > 1 ; ) {
    temp = new_node(znull);
    temp->right = (*np++);
    temp->left = (*np);
    *np = temp;
    *(fp+1) = *(fp+1) + *fp;
    *fp++ = 0;
    sort_freq(fp,np,num_freq);
  }

  return temp;
}

static NODE *new_node(value)
/****************************************************************************
*_TITLE new_node - allocates a NODE structure and returns a pointer to it   *
*_ARGS  TYPE        NAME        I/O     DESCRIPTION                         */
        short int   value;    /* I      Value to assign to DN field         */

{
  NODE *temp;         /* Pointer to the memory block */

/***************************************************************************
  Allocate the memory and intialize the fields.
****************************************************************************/

  temp = (NODE *) lmalloc(sizeof(NODE));
  temp->right = NULL;
  temp->dn = value;
  temp->left = NULL;
  return temp;
}

void sort_freq(freq_list,node_list,num_freq)
/****************************************************************************
*_TITLE sort_freq - sorts frequency and node lists in increasing freq. order*
*_ARGS  TYPE       NAME            I/O  DESCRIPTION                         */
     long int   *freq_list;   /* I   Pointer to frequency list           */
     NODE       **node_list;  /* I   Pointer to array of node pointers   */
     long int   num_freq;     /* I   Number of values in freq list       */

{
  /* Local Variables */
  register long int *i;       /* primary pointer into freq_list */
  register long int *j;       /* secondary pointer into freq_list */

  register NODE **k;          /* primary pointer to node_list */
  register NODE **l;          /* secondary pointer into node_list */

  long int temp1;             /* temporary storage for freq_list */
  NODE *temp2;                /* temporary storage for node_list */

  register long int cnt;      /* count of list elements */


/************************************************************************
  Save the current element - starting with the second - in temporary
  storage.  Compare with all elements in first part of list moving
  each up one element until the element is larger.  Insert current
  element at this point in list.
*************************************************************************/

  if (num_freq <= 0)
    return;      /* If no elements or invalid, return */

  for (i=freq_list, k=node_list, cnt=num_freq ; --cnt ; *j=temp1, *l=temp2) {
    temp1 = *(++i);
    temp2 = *(++k);

    for (j = i, l = k ;  *(j-1) > temp1 ; ) {
      *j = *(j-1);
      *l = *(l-1);
      j--;
      l--;
      if ( j <= freq_list)
	break;
    }
  }
  return;
}

/* free_tree - free memory of all allocated nodes
 *
 * This routine is supplied to the programmer to free up all the
 * allocated memory required to build the huffman tree.  The count
 * of the nodes freed is returned in the parameter 'nfreed'.  The
 * purpose of the routine is so if the user wishes to decompress more
 * than one file per run, the program will not keep allocating new
 * memory without first deallocating all previous nodes associated
 * with the previous file decompression.
 *
 * 16-AUG-89 Kris Becker   USGS, Flagstaff Original Version
 */

static void free_tree(nfreed)
     long int   *nfreed;  /* Return of total count of nodes freed. */
{
  long int total_free = 0;

  /* Simply call the free_node routine and return the result.
   */

  *nfreed = free_node(tree,total_free);
  return;
}

/* free_node - deallocates an allocated NODE pointer
 *
 * free_node will check both right and left pointers of a node
 * and then free the current node using the free() C utility.
 * Note that all nodes attached to the node via right or left
 * pointers area also freed, so be sure that this is the desired
 * result when calling this routine.
 *
 * This routine is supplied to allow successive calls to the
 * decmpinit routine.  It will free up the memory allocated
 * by previous calls to the decmpinit routine.  The call to free
 * a previous huffman tree is:  total = free_node(tree,(long) 0);
 * This call must be done by the programmer application routine
 * and is not done by any of these routines.
 *
 * 16-AUG-89  Kris Becker U.S.G.S  Flagstaff Original Version
 */

static long int free_node(pnode,total_free)
        NODE     *pnode;       /* I  Pointer to node to free               */
        long int total_free;   /* I  Total number of freed nodes           */
{
  if (pnode == (NODE *) NULL)
    return(total_free);
	
  if (pnode->right != (NODE *) NULL)
    total_free = free_node(pnode->right,total_free);
  if (pnode->left != (NODE *) NULL)
    total_free = free_node(pnode->left,total_free);

  lfree((byte *) pnode);
  return(total_free + 1);
}

/* pdsDecompress - decompresses Huffman coded compressed image lines
 */
static int decompress(ibuf,obuf,nin,nout)
     char       *ibuf;   /* I        Compressed data buffer              */
     char       *obuf;   /* O        Decompressed image line             */
     long int   *nin;    /* I        Number of bytes on input buffer     */
     long int   *nout;   /* I        Number of bytes in output buffer    */

{
  /* Local Variables */
  register NODE *ptr = tree;        /* pointer to position in tree */
  register unsigned char test;      /* test byte for bit set */
  register unsigned char idn;       /* input compressed byte */

  register char odn;                /* last dn value decompressed */

  char *ilim = ibuf + *nin;         /* end of compressed bytes */
  char *olim = obuf + *nout;        /* end of output buffer */
  char *obuf_start= obuf;

  /* Check for valid input values for nin, nout and make initial assignments.
   */

  if (ilim > ibuf && olim > obuf)
    odn = *obuf++ = *ibuf++;
  else {
    fprintf(stderr,"\nInvalid byte count in dcmprs!\n");
    return(-1);
  }

  /* Decompress the input buffer.  Assign the first byte to the working
   * variable, idn.  An arithmatic and (&) is performed using the variable
   * 'test' that is bit shifted to the right.  If the result is 0, then
   * go to right else go to left.
   */

  for (idn=(*ibuf) ; ibuf < ilim  ; idn =(*++ibuf)) {
    for (test=0x80 ; test ; test >>= 1) {
      ptr = (test & idn) ? ptr->left : ptr->right;

      if (ptr->dn != -1) {
	if (obuf >= olim)
	  return(obuf - obuf_start);
	odn -= ptr->dn + 256;
	*obuf++ = odn;
	ptr = tree;
      }
    }
  }
  return(obuf - obuf_start);
}

/*********************************************************************/
/*                                                                   */
/* subroutine read_var - read variable length records from input file*/
/*                                                                   */
/*********************************************************************/

static int read_var(zf, ibuf)
     ZFILE *zf;
     char  *ibuf;
{
  int   length,result,nlen;
  unsigned int value;
  unsigned char buf[2];

  length = 0;
  result = zread(zf,buf,2);
  length= memToValLSB(buf, 2);
  nlen = zread(zf,(byte *)ibuf,length+(length%2));
  return(length);
}

skip_record(zf)
     ZFILE *zf;
{
  unsigned int value;
  unsigned char buf[2];
  char *dummy;

  zread(zf, buf, 2);
  value= memToValLSB(buf, 2);
  dummy= (char *)lmalloc(value);
  zread(zf, (byte *)dummy, value);
  lfree((byte *)dummy);
}

/* initialize pdsRead() stuff.  if this is a huffman-encoded file, this
 * reads the histogram table and initializes the huffman tree.
 */

void pdsInit(zf, type)
     ZFILE *zf;
     int type;
{
  if (type == PDSVARIABLE) {
    char buf[2];
    int len;
    char *dummy;

    Decompressing= 1; /* flag pdsRead() to do decompressions */

    /* skip label and image histogram
     */
    skip_record(zf);
    skip_record(zf);

    /* read in huffman encoding histogram
     */

    read_var(zf, (char *)hist);
    read_var(zf, (char *)hist+836);
    read_var(zf, (char *)hist+1672);

    tree = huff_tree(hist);
  }
  else
    Decompressing= 0;
}

/* function which will read pds data and do decompression if necessary.
 */

int pdsRead(zf, buf, size)
     ZFILE *zf;
     char *buf;
     int size;
{
  static char obuf[2048];
  char ibuf[2048];
  int left;
  int in_length;
  static int out_length= 2048, out_ptr= 0;

  if (Decompressing) {
    left= size;
    while (left) {
      if (out_ptr == out_length) {
	in_length= read_var(zf, ibuf);
	if (in_length < 0)
	  return(-1);
	out_length= decompress(ibuf, obuf, &in_length, &out_length);
	if (out_length < 0)
	  return(-1);
	out_ptr= 0;
      }
      if (left <= (out_length - out_ptr)) {
	bcopy(obuf + out_ptr, buf, size);
	out_ptr += left;
	return(size);
      }
      else {
	bcopy(obuf + out_ptr, buf, out_length - out_ptr);
	left= left - (out_length - out_ptr);
	out_ptr= out_length;
      }
    }
    return(size);
  }
  else
    return(zread(zf, (byte *)buf, size));
}
