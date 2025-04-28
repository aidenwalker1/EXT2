/************* open_close_lseek.c file **************/

#ifndef __OPENCLOSELSEEK_C__
#define __OPENCLOSELSEEK_C__

#include "mkdir_creat.c"

// clears out minode blocks
int truncate(MINODE *mip)
{
  // goes through, deallocs each block
  for (int i = 0; i < 15; i++) {
    if (mip->INODE.i_block[i] == 0) {
      break;
    }
    bdalloc(mip->dev, mip->ino);
    mip->INODE.i_block[i] = 0;
  }

  // updates atime, size
  mip->INODE.i_atime = time(NULL);
  mip->INODE.i_size = 0;
  mip->dirty = 1;
}

// opens file for r/w/a
int open_file(char *pathname, int mode)
{
  if (mode < 0 || mode > 3) {
    printf("mode %d out of bounds\n", mode);
    return -1;
  }
  int dev; // local in function that calls getino()
  if (pathname[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;        // relative pathname
  int ino = getino(pathname, &dev); // pass &dev as an extra parameter

  // checks if need to create file
  if (ino == 0)
  {
    if (pathname[0] == '/')
      dev = root->dev; // absolute pathname
    else
      dev = running->cwd->dev;
    // creates new file
    creat_file(pathname);
    ino = getino(pathname, &dev);
  }

  MINODE *mip = iget(dev, ino);

  // allocates new oft
  OFT *op = (OFT *)malloc(sizeof(OFT));

  op->mode = mode;
  op->minodeptr = mip;

  // checks if append
  if (mode == 3)
  {
    op->offset = mip->INODE.i_size;
  }
  else
  {
    op->offset = 0;
  }
  //adds 1 ref
  op->refCount = 1;

  // looks for open fd to set oft
  for (int i = 0; i < 16; i++)
  {
    if (running->fd[i] == NULL)
    {
      running->fd[i] = op;
      return i;
    }
  }

  printf("no open slots\n");
  return -1; // Eventually: return file descriptor of opened file
}

// closes file
int my_close(int fd)
{
  // makes sure fd in range
  if (fd > -1 && fd < 16)
  {
    // checks that fd is open
    if (running->fd[fd] != 0)
    {
      // decrements ref count
      running->fd[fd]->refCount--;

      // if no more opens, put mip
      if (running->fd[fd]->refCount == 0)
      {
        iput(running->fd[fd]->minodeptr);
      }
    }

    // free fd
    free(running->fd[fd]);
    running->fd[fd] = 0;
    return 1;
  }

  return -1;
}

// moves position in file
int my_lseek(int fd, int position)
{
  if (fd < 0 || fd > 15) {
    printf("%d out of range\n", fd);
    return;
  }

  // gets oft
  OFT *op = running->fd[fd];

  // makes sure fd is open
  if (op == NULL)
  {
    return -1;
  }

  int oldpos = op->offset;

  // makes sure position in file range
  if (position <= op->minodeptr->INODE.i_size)
  {

    op->offset = position;
  }

  return oldpos; // Eventually: return original position in file
}

// prints fds
int pfd()
{
  printf(" fd     mode    offset    INODE\n");

  // goes through and prints all fds
  for (int i = 0; i < 16; i++) {
    OFT* op = running->fd[i];
    if (op != 0) {
      printf("%d     %d     %d     [%d, %d]\n", i, op->mode, op->offset, op->minodeptr->dev, op->minodeptr->ino);
    }
  }
  return 1;
}

//dups fd
int dup(int fd)
{
  // checks in bounds
  if (fd < -1 || fd > 15) {
    printf("fd %d out of bounds", fd);
    return -1;
  }

  // checks already open
  if (running->fd[fd] == 0) {
    printf("%d not open",fd);
    return -1;
  }

  OFT* copy = running->fd[fd];

  // finds open slot
  for (int i = 0; i < 16; i++) {
    if (running->fd[i] == 0) {
      // copies fd into empty slot
      OFT* newoft = malloc(sizeof(OFT));
      memcpy(newoft, copy, sizeof(OFT));
      running->fd[i] = newoft;
      running->fd[fd]->refCount++;
      return i;
    }
  }
  printf("no open slots\n");
  return -1;
}

// copies fd into gd
int dup2(int fd, int gd)
{
  // checks for fd/gd bounds
  if (fd < -1 || fd > 15) {
    printf("fd %d out of bounds", fd);
    return -1;
  }

  if (gd < -1 || gd > 15) {
    printf("gd %d out of bounds", gd);
    return -1;
  }

  //makes sure fd is open
  if (running->fd[fd] == 0) {
    printf("%d not open",fd);
    return -1;
  }
  OFT* copy = running->fd[fd];

  // checks if need to close gd
  if (running->fd[gd] != 0) {
    free(running->fd[gd]);
  }
  
  // copies fd into gd
  OFT* newoft = malloc(sizeof(OFT));
  memcpy(newoft, copy, sizeof(OFT));
  running->fd[gd] = newoft;
  running->fd[fd]->refCount++;
  return gd;
}

#endif