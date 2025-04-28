/************* mount_umount.c file **************/

#ifndef __MOUNTUMOUNT_C__
#define __MOUNTUMOUNT_C__

#include "mkdir_creat.c"

// mounts dev pathname on mount point
int mount(char *pathname, char* mount_point)
{
  // if just want to print mount name
  if (*pathname == 0)
  {
    printf("mounted on %s\n", get_mtable(running->cwd->dev)->devName);
    return;
  }

  int found = 0;
  int first_index = -1;

  // checks that mount point isnt already mounted
  for (int i = 0; i < 10; i++)
  {
    // gets first index for new mtable
    if (mtable[i].dev == 0)
    {
      if (first_index == -1)
      {
        first_index = i;
      }
      continue;
    }
    else
    {
      // checks point already mounted
      if (strcmp(mount_point, mtable[i].mntName) == 0)
      {
        found = 1;
      }
    }
  }

  if (!found && first_index != -1)
  {
    int i;
    MTABLE *mp;
    SUPER *sp;
    GD *gp;
    char buf[BLKSIZE];
    char sbuf[BLKSIZE];
    int newdev = -1;

    // gets dev of disk
    
    newdev = open(pathname, O_RDWR);
    

    if (newdev < 0)
    {
      printf("panic : can’t open root device\n");
      return;
    }

    /* get super block of rootdev */
    get_block(newdev, 1, sbuf);
    sp = (SUPER *)sbuf;
    /* check magic number */
    if (sp->s_magic != SUPER_MAGIC)
    {
      printf("super magic=%x : %s is not an EXT2 filesys\n",
             sp->s_magic, rootdev);
      return;
    }
    get_block(newdev, 2, buf);
    gp = (GD *)buf;

    MINODE *mip = mntiget(newdev, 2, gp->bg_inode_table);

    // makes sure mount point is dir
    if (!S_ISDIR(mip->INODE.i_mode)) {
       printf("%s is not dir\n", mount_point);
       close(pathname);
       iput(mip);
       return -1;
    }

    // makes sure mount point isnt a cwd or already mounted
    for (int i = 0; i < 10; i++)
    {
      if (mtable[i].dev != 0)
      {
        if (mtable[i].mntDirPtr == mip || running->cwd == mip)
        {
          printf("%s is busy\n", pathname);
          close(pathname);
          iput(mip);
          return -1;
        }
      }
    }

    mp = &mtable[first_index]; // use mtable

    mp->dev = newdev;
    // copy super block info into mtable
    mp->ninodes = sp->s_inodes_count;
    mp->nblocks = sp->s_blocks_count;
    strcpy(mp->devName, pathname);
    strcpy(mp->mntName, mount_point);
    mp->bmap = gp->bg_block_bitmap;
    mp->imap = gp->bg_inode_bitmap;
    mp->iblock = gp->bg_inode_table;
    printf("bmap=%d imap=%d iblock=%d\n", mp->bmap, mp->imap, mp->iblock);

    // sets mip to mounted
    mip->mounted = 1;

    mp->mntDirPtr = mip; // double link
    mip->mptr = mp;

    printf("mount : %s mounted on %s \n", pathname, mount_point);
  }
  else
  {
    printf("%s already mounted\n", pathname);
  }

  return 1;
}

//unmounts pathname
int umount(char *pathname)
{
  int index = -1;

  // looks for dev name in mtables
  for (int i = 0; i < 10; i++)
  {
    if (mtable[i].dev != 0)
    {
      if (strcmp(pathname, mtable[i].devName) == 0)
      {
        index = i;
        break;
      }
    }
  }

  // if table found
  if (index != -1) {
    MTABLE* mp = &mtable[index];
    
    MINODE* mip = mp->mntDirPtr;

    // checks if any minode in use
    for (int i = 0; i < NMINODE; i++) {
      if (minode[i].dev == mp->dev) {
        printf("node in use\n");
        iput(mip);
        return;
      }
    }
    mp->dev = 0;
    mip->mounted = 0;
    mip->dirty = 1;
    iput(mip);
  }
  else {
    printf("%s not found\n", pathname);
  }

  return 0;
}

#endif