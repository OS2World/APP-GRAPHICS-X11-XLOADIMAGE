/* packtar.c:
 *
 * this generates a version-specific tar file by creating a shadow tree
 * containing every file listed on the command line.
 *
 * note that whole directories cannot be handled at once; each individual
 * file needs to be specified.
 */

#include "patchlevel"
#include <stdio.h>
#include <errno.h>

extern int errno;

/* poor-man's varargs.  good enough for now.
 */
int run(a0, a1, a2, a3, a4, a5, a6, a7, a8)
     char *a0, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
{
  int status;
  int pid;

  pid = fork();
  switch (pid) {
  case -1:
    perror("fork");
    return;
  case 0:
    execlp(a0, a0, a1, a2, a3, a4, a5, a6, a7, a8);
    perror(a0);
    exit(1);
  default:
    while ((waitpid(pid, &status, 0) < 0) && (errno == EINTR))
      /* EMPTY */
      ;
    return(status);
  }
}

/* this creates a link for a file.  for every directory name in the
 * file name it creates the appropriate directory.
 */
void link_file(dir, old_file)
     char *dir;
     char *old_file;
{
  char new_file[1024];
  char *p;

  strcpy(new_file, dir); /* target directory */
  strcat(new_file, "/");
  strcat(new_file, old_file);

  for (p = new_file; p = strchr(p, '/'); p++) {
    *p = '\0'; /* stomp directory separator */
    if ((mkdir(new_file, 0775) < 0) && (errno != EEXIST)) {
      perror(new_file);
      return;
    }
    *p = '/'; /* repair directory separator */
  }

  /* link the two files
   */
  if (link(old_file, new_file) < 0)
    perror(new_file);
}

main(argc, argv)
     int argc;
     char *argv[];
{
  char dir_name[1024];
  char tar_name[1024];
  int i;

  /* create subdirectory with an appropriate name
   */
  sprintf(dir_name, "xloadimage.%s.%s", VERSION, PATCHLEVEL);
  sprintf(tar_name, "xloadimage.%s.%s.tar", VERSION, PATCHLEVEL);
  if (mkdir(dir_name, 0775) < 0) {
    perror(dir_name);
    fprintf(stderr, "Couldn't create tar sub directory, sorry!\n");
    exit(1);
  }

  /* create shadow in the subdirectory
   */
  printf("Creating shadow tree in %s...\n", dir_name);
  for (i = 1; i < argc; i++)
    link_file(dir_name, argv[i]);

  /* invoke 'tar' on the shadow tree
   */
  printf("Creating tar file %s....\n", tar_name);
  if (run("tar", "-cf", tar_name, dir_name, NULL) != 0)
    exit(1);

  /* clean up the shadow tree
   */
  printf("Cleaning up shadow tree....\n");
  if (run("rm", "-rf", dir_name, NULL) != 0)
    exit(1);

  printf("You now have your very own bouncing baby tar file.\n");
  exit(0); /* all's well that ends well */
}
