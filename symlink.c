/************* symlink.c file **************/

#ifndef __SYMLINK_C__
#define __SYMLINK_C__

#include "link_unlink.c"

// creates symlink from newpath to oldpath
int symlink_file(char *oldpath, char *newpath)
{
  int dev;
  if (oldpath[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;
  int oino = getino(oldpath, &dev);

  // makes sure old path exists
  if (oino == 0)
  {
    printf("%s doesnt exist\n", oldpath);
    return;
  }

  if (newpath[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;
  int nino = getino(newpath, &dev);

  // makes sure newpath doesnt exist
  if (nino != 0)
  {
    printf("%s already exists\n", newpath);
    return;
  }

  // creates file from newpath
  creat_file(newpath);

  if (newpath[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;
  
  // sets mode to link
  int linkino = getino(newpath, &dev);
  MINODE *newmip = iget(dev, linkino);
  newmip->INODE.i_mode = 0xA000;

  char *base = basename(newpath);

  if (base[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;
  int parentino = getino(base, &dev);

  MINODE *parent = iget(dev, parentino);

  enter_name(newmip, linkino, oldpath);  ////// look out here

  // sets size
  newmip->INODE.i_size = strlen(oldpath);

  parent->dirty = 1;
  newmip->dirty = 1;
  iput(parent);
  iput(newmip);

  return 1;
}

// reads file link into buf
int my_readlink(char *pathname, char buffer[1024])
{
  int dev;
  if (pathname[0] == '/')
    dev = root->dev; // absolute pathname
  else
    dev = running->cwd->dev;
  int ino = getino(pathname, &dev);

  // makes sure path exists
  if (ino == 0)
  {
    printf("%s doesnt exist", pathname);
    return -1;
  }

  MINODE *mip = iget(dev, ino);

  // makes sure path is link
  if (!S_ISLNK(mip->INODE.i_mode))
  {
    printf("%s isnt link", pathname);
    iput(mip);
    return -1;
  }

  char buf[1024];
  char *cp;
  DIR *dp;

  // looks for link name
  for (int i = 0; i < 15; i++)
  {
    if (mip->INODE.i_block[i] == 0)
    {
      break;
    }

    get_block(mip->dev, mip->INODE.i_block[i], buf);

    // skips first 2 entries
    dp = (DIR *)buf;
    cp = buf;
    cp += dp->rec_len;
    dp = (DIR *)cp;
    cp += dp->rec_len;
    dp = (DIR *)cp;

    while (cp < buf + BLKSIZE)
    {
      // checks if current entry is wanted file
      if (dp->inode == ino)
      {
        MINODE *dest = iget(dev, dp->inode);

        // saves size and name
        int size = dest->INODE.i_size;
        strncpy(buffer, dp->name, dp->name_len);

        iput(dest);
        iput(mip);

        return size;
      }
      
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  iput(mip);

  return -1;
}

#endif