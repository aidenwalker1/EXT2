/************* misc1.c file **************/

#ifndef __MISC1_C__
#define __MICS1_C__

#include "write_cp.c"

int menu()
{
  printf("\t\t******************** Menu *******************\n");
  printf("\t\tmkdir\tcreat\tmount\tumount\trmdir\n");
  printf("\t\tcd\tls\tpwd\tstat\trm\n");
  printf("\t\tlink\tunlink\tsymlink\tchmod\tchown\ntouch\n");
  printf("\t\topen\tpfd\tlseek\tclose\n");
  printf("\t\tread\twrite\tcat\tcp\tmv\n");
  printf("\t\tcs\tfork\tps\tkill\tquit\n");
  printf("\t\t=============   Usage Examples ==============\n");
  printf("\t\tmkdir\tfilename\n");
  printf("\t\tmount\tfilesys\t/mnt\n");
  printf("\t\tchmod\tfilename\t0644\n");
  printf("\t\topen\tfilename\tmode (0|1|2|3 for R|W|RW|AP)\n");
  printf("\t\twrite\tfd\ttext_string\n");
  printf("\t\tread\tfd\tnbytes\n");
  printf("\t\tpfd\t(display opened file descriptors)\n");
  printf("\t\tcs\t(switch process)\n");
  printf("\t\tfork\t(fork child process)\n");
  printf("\t\tps\t(show process queue as Pi[uid]==>)\n");
  printf("\t\tkill\tpid\t(kill a process)\n");
  printf("\t\t*********************************************\n");
}

// gets stat info
struct stat my_stat(char *pathname)
{
  struct stat myst;
  int dev; // local in function that calls getino()
  if (pathname[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;        // relative pathname
  int ino = getino(pathname, &dev); // pass &dev as an extra parameter

  // checks path exists
  if (ino == 0)
  {
    printf("%s doesnt exist", pathname);
    return;
  }

  MINODE *mip = iget(dev, ino);
  INODE n = mip->INODE;

  // time info
  struct timespec at = {n.i_atime, 0};
  struct timespec ct = {n.i_ctime, 0};
  struct timespec mt = {n.i_mtime, 0};

  // gets mip and mip inode info
  myst.st_atim = at;
  myst.st_blocks = n.i_blocks;
  myst.st_ctim = ct;
  myst.st_dev = mip->dev;
  myst.st_gid = n.i_gid;
  myst.st_ino = mip->ino;
  myst.st_mode = n.i_mode;
  myst.st_mtim = mt;
  myst.st_nlink = n.i_links_count;
  myst.st_size = n.i_size;
  myst.st_uid = n.i_uid;
  myst.st_blksize = BLKSIZE;
  myst.st_rdev = mip->dev;

  iput(mip);
  return myst;
}

// changes path mode
int my_chmod(char *pathname, int mode)
{
  int dev; // local in function that calls getino()
  if (pathname[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;        // relative pathname
  int ino = getino(pathname, &dev); // pass &dev as an extra parameter

  // makes sure path exists
  if (ino == 0)
  {
    printf("%s doesnt exist", pathname);
    return;
  }

  // gets path inode, sets mode to input
  MINODE *mip = iget(dev, ino);
  mip->INODE.i_mode = mode;
  mip->dirty = 1;
  iput(mip);
  return 1;
}

// changes atime to current time
int my_utime(char *pathname)
{
  int dev; // local in function that calls getino()
  if (pathname[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;        // relative pathname
  int ino = getino(pathname, &dev); // pass &dev as an extra parameter

  // checks path exists
  if (ino == 0)
  {
    printf("%s doesnt exist", pathname);
    return;
  }

  MINODE *mip = iget(dev, ino);

  // sets atime to current time
  mip->INODE.i_atime = time(NULL);
  mip->dirty = 1;
  iput(mip);

  return 1;
}

int my_chown(char *pathname, int uid)
{
  return 1;
}

int cs()
{
  return 1;
}

int fork()
{
  return 1;
}

int ps(char* pathname)
{
  if (*pathname == '0') {
    running = &proc[0];
  }
  else {
    running = &proc[1];
  }
  return 1;
}

int kill(int pid)
{
  return 1;
}

// moves file1 to file2
int my_mv(char *pathname1, char *pathname2)
{
  int dev1; // local in function that calls getino()
  if (pathname1[0] == '/')
    dev1 = root->dev; // absolute pathname
  else
    dev1 = running->cwd->dev;          // relative pathname
  int ino1 = getino(pathname1, &dev1); // pass &dev as an extra parameter

  // checks first path exists
  if (ino1 == 0)
  {
    printf("%s doesnt exist\n", pathname1);
    return;
  }

  int dev2; // local in function that calls getino()
  if (pathname2[0] == '/')
    dev2 = root->dev; // absolute pathname
  else
    dev2 = running->cwd->dev;          // relative pathname
  int ino2 = getino(pathname2, &dev2); // pass &dev as an extra parameter

  // checks second path doesnt exist
  if (ino2 != 0)
  {
    printf("%s exists already\n", pathname2);
    return;
  }

  if (pathname2[0] == '/')
    dev2 = root->dev; // absolute pathname
  else
    dev2 = running->cwd->dev; // relative pathname
  getino(pathname2, &dev2);   // pass &dev as an extra parameter

  // if on same device, link otherwise cp
  if (dev2 == dev1)
  {
    link_file(pathname1, pathname2);
    my_unlink(pathname1);
  }
  else
  {
    my_cp(pathname1, pathname2);
  }

  return 1;
} 

#endif