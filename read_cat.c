/************* read_cat.c file **************/

#ifndef __READCAT_C__
#define __READCAT_C__

#include "open_close.c"

// reads into buf
int my_read(int fd, char buf[], int nbytes)
{
  int count = 0;

  // makes sure fd in range
  if (fd > -1 && fd < 16)
  {
    // makes sure fd is open and mode is read
    if (running->fd[fd] != 0 && running->fd[fd]->mode == 0)
    {
      OFT *op = running->fd[fd];
      MINODE *mip = op->minodeptr;

      // gets available data
      int avil = mip->INODE.i_size - op->offset;
      char kbuf[1024];
      memset(kbuf, 0, 1024);

      while (nbytes && avil)
      {
        // gets block
        int lbk = op->offset / BLKSIZE;
        int blk = map(lbk, mip);

        int start = op->offset % BLKSIZE;

        get_block(mip->dev, blk, kbuf);

        char *cp = kbuf + start;
        int remain = BLKSIZE - start;

        // find min of remain, avil, nbytes
        int min = remain;
        if (min > avil)
        {
          min = avil;
        }
        if (min > nbytes)
        {
          min = nbytes;
        }
        memcpy(buf, cp, min);

        // updates offset
        op->offset += min;

        // updates char pointers
        buf += min;
        cp += min;

        // updates byte count
        avil -= min;
        nbytes -= min;

        // updates read count
        count += min;
      }
    }
  }
  return count; // Eventually: Return the actual number of bytes read
}

// maps lbk to blk for mip
int map(int lbk, MINODE* mip)
{
  int blk;

  // direct blocks
  if (lbk < 12)
  {
    blk = mip->INODE.i_block[lbk];

    // allocate new block
    if (blk == 0) {
      blk = balloc(mip->dev);
      mip->INODE.i_block[lbk] = blk;
    }
  }
  else if (12 <= lbk < 12 + 256)
  { // indirect blocks
    int ibuf[256];
    char emptybuf[1024];
    memset(emptybuf, 0, 1024);
    memset(ibuf, 0, 256);

    // allocate new block
    if (mip->INODE.i_block[12] == 0) {
      mip->INODE.i_block[12] = balloc(mip->dev);
      put_block(mip->dev, mip->INODE.i_block[12], emptybuf);
    }
    // get block at indirect iblock
    get_block(mip->dev, mip->INODE.i_block[12], ibuf);

    // get block num
    blk = ibuf[lbk - 12];

    // allocate new block
    if (blk == 0) {
      ibuf[lbk - 12] = balloc(mip->dev);
      blk = ibuf[lbk - 12];
      put_block(mip->dev, mip->INODE.i_block[12], ibuf);
    }
  }

  // cant do else here???? only works if i do a separate if 
  if (lbk >= 12 + 256)
  { // doube indirect blocks; see Exercise 11.13 below.
    int ibuf[256];
    char emptybuf[1024];
    memset(emptybuf, 0, 1024);
    memset(ibuf, 0, 256);

    // allocate new block
    if (mip->INODE.i_block[13] == 0) {
      mip->INODE.i_block[13] = balloc(mip->dev);
      put_block(mip->dev, mip->INODE.i_block[13], emptybuf);
    }
    // get block at double indirect iblock
    get_block(mip->dev, mip->INODE.i_block[13], ibuf);

    // shift lbk
    lbk -= (12 + 256);

    // get first block as first part of mailman
    int first_blk = ibuf[lbk / 256];

    // allocate new block
    if (first_blk == 0) {
      ibuf[lbk / 256] = balloc(mip->dev);
      first_blk = ibuf[lbk / 256];
      put_block(mip->dev, first_blk, emptybuf);
      put_block(mip->dev, mip->INODE.i_block[13], ibuf);
    }
    memset(ibuf, 0, 256);

    // get indirect block from first block
    get_block(mip->dev, first_blk, ibuf);

    // get second block as second part of mailman
    blk = ibuf[lbk % 256];

    // allocate new block
    if (blk == 0) {
      ibuf[lbk % 256] = balloc(mip->dev);
      blk = ibuf[lbk % 256];
      put_block(mip->dev, blk, emptybuf);
      put_block(mip->dev, first_blk, ibuf);
    }
  }

  return blk;
}

// reads file contents
int cat_file(char *pathname)
{
  int dev;
  if (pathname[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;
  int ino = getino(pathname, &dev);

  // makes sure file exists
  if (ino == 0)
  {
    printf("%s doesnt exist\n", pathname);
    return;
  }

  // gets file, opens it
  MINODE *mip = iget(dev, ino);
  int fd = open_file(pathname, 0);

  char buf[1024];
  
  int rd = -1;

  // run while stuff to read
  while (rd != 0)
  {
    // reset buf, get data
    memset(buf, 0, 1024);

    rd = my_read(fd, buf, 1024);

    // prints data to screen
    for (int i = 0; i < rd; i++)
    {
      printf("%c", buf[i]);
    }
  }

  printf("\n");

  // closes file
  my_close(fd);
  iput(mip);
}

#endif