/****************************************************************************
 *              A_W testing ext2 file system                       *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"
#include "cd_ls_pwd.c"
#include "symlink.c"
#include "write_cp.c"
#include "mount_umount.c"
#include "misc1.c"

char gpath[128]; // global for tokenized components
char *pname[32]; // assume at most 32 components in pathname
int n;           // number of component strings
char pathname[128];

int fd;

int fs_init()
{
  int i, j;
  for (i = 0; i < NMINODE; i++) // initialize all minodes as FREE
    minode[i].refCount = 0;

  for (i = 0; i < NMTABLE; i++) // initialize mtables as FREE
    mtable[i].dev = 0;

  for (i = 0; i < NOFT; i++) // initialize ofts as FREE
    oft[i].refCount = 0;

  for (i = 0; i < NPROC; i++)
  { // initialize PROCs

    proc[i].status = READY; // ready to run

    proc[i].pid = i; // pid = 0 to NPROC-1

    proc[i].uid = i; // P0 is a superuser process

    for (j = 0; j < NFD; j++)
      proc[i].fd[j] = 0; // all file descriptors are NULL

    proc[i].next = &proc[i + 1]; // link list
  }

  proc[NPROC - 1].next = &proc[0]; // circular list

  running = &proc[0]; // P0 runs first
}

int mount_root(char *rootdev, char *rootname)
{
  int i;
  MTABLE *mp;
  SUPER *sp;
  GD *gp;
  char buf[BLKSIZE];

  globalDev = open(rootdev, O_RDWR);

  if (globalDev < 0)
  {
    printf("panic : can’t open root device\n");
    exit(1);
  }
  /* get super block of rootdev */
  get_block(globalDev, 1, buf);
  sp = (SUPER *)buf;
  /* check magic number */
  if (sp->s_magic != SUPER_MAGIC)
  {
    printf("super magic=%x : %s is not an EXT2 filesys\n",
           sp->s_magic, rootdev);
    exit(0);
  }

  // fill mount table mtable[0] with rootdev information
  mp = &mtable[0]; // use mtable[0]
  mp->dev = globalDev;
  // copy super block info into mtable[0]
  ninodes = mp->ninodes = sp->s_inodes_count;
  nblocks = mp->nblocks = sp->s_blocks_count;
  strcpy(mp->devName, rootdev);
  strcpy(mp->mntName, rootname);
  get_block(globalDev, 2, buf);
  gp = (GD *)buf;
  bmap = mp->bmap = gp->bg_block_bitmap;
  imap = mp->imap = gp->bg_inode_bitmap;
  iblock = mp->iblock = gp->bg_inode_table;
  printf("bmap=%d imap=%d iblock=%d\n", bmap, imap, iblock);

  // call iget(), which inc minode’s refCount

  root = iget(globalDev, 2); // get root inode

  mp->mntDirPtr = root; // double link
  root->mptr = mp;
  // set proc CWDs
  for (i = 0; i < NPROC; i++)         // set proc’s CWD
    proc[i].cwd = iget(globalDev, 2); // each inc refCount by 1

  printf("mount : %s mounted on %s \n", rootdev, rootname);

  return 0;
}

int quit()
{
  int i;

  for (i = 0; i < NMINODE; i++)
  {
    MINODE *mip = &minode[i];
    if (mip->refCount && mip->dirty)
    {
      mip->refCount = 1;
      iput(mip);
    }
  }
  close(running->cwd->mptr->devName);
  exit(0);
}

int main(int argc, char *argv[], char **env)
{
  int ino, linefd, linep, lineposition, linenbytes, lineuid, mode;
  char buf[BLKSIZE];
  char line[128], cmd[32];

  char pathname2[128];

  char diskname[128];

  char *disk = rootdev;

  printf("\n\n\nIf you've gotten here, congratulations! The program has compiled.\n\n");
  printf("There are several utility functions that are not yet complete-\nwhich means beyond this point, you won't be able to properly load up the filesystem.\n\n");
  printf("Take a look around at the comments to figure out what's missing-\nwhere you can find it, and what you're going to have to do yourself.\n\n");
  printf("Good luck and take care!\n\n\n\n");

  fs_init();

  // dont try this it will break
  if (argc > 1)
  {
    disk = argv[1];

    rootdev = disk;

    mount_root(rootdev, "/");
  }
  else
  {
    mount_root(rootdev, "/");
  }

  while (1)
  {
    // resets line
    memset(line, 0, 128);
    memset(pathname, 0, 128);
    memset(pathname2, 0, 128);
    memset(cmd, 0, 32);

    // gets line
    printf("P%d running: ", running->pid);
    printf("input command : ");
    fgets(line, 128, stdin);
    line[strlen(line) - 1] = 0;

    if (line[0] == 0)
      continue;

    sscanf(line, "%s %s", cmd, pathname);

    // runs command
    if (!strcmp(cmd, "ls"))
    {
      my_ls(pathname);
    }
    if (!strcmp(cmd, "cd"))
    {
      chdir(pathname);
    }
    if (!strcmp(cmd, "pwd"))
    {
      my_pwd(running->cwd);
    }
    if (!strcmp(cmd, "creat"))
    {
      creat_file(pathname);
    }
    if (!strcmp(cmd, "mkdir"))
    {
      make_dir(pathname);
    }
    if (!strcmp(cmd, "rmdir"))
    {
      remove_dir(pathname);
    }
    if (!strcmp(cmd, "unlink"))
    {
      my_unlink(pathname);
    }
    if (!strcmp(cmd, "link"))
    {
      sscanf(line, "%s %s %s", cmd, pathname, pathname2);
      link_file(pathname, pathname2);
    }
    if (!strcmp(cmd, "symlink"))
    {
      sscanf(line, "%s %s %s", cmd, pathname, pathname2);
      symlink_file(pathname, pathname2);
    }
    if (!strcmp(cmd, "cat"))
    {
      cat_file(pathname);
    }
    if (!strcmp(cmd, "cp"))
    {
      sscanf(line, "%s %s %s", cmd, pathname, pathname2);
      my_cp(pathname, pathname2);
    }
    if (!strcmp(cmd, "mv"))
    {
      sscanf(line, "%s %s %s", cmd, pathname, pathname2);
      my_mv(pathname, pathname2);
    }
    if (!strcmp(cmd, "mount"))
    {
      sscanf(line, "%s %s %s", cmd, pathname, pathname2);
      mount(pathname, pathname2);
    }
    if (!strcmp(cmd, "umount"))
    {
      umount(pathname);
    }
    if (!strcmp(cmd, "open"))
    {
      int mode = -1;
      sscanf(line, "%s %s %d", cmd, pathname, mode);
      int fd = open_file(pathname, 1);
      char *test = "test string";
      write_file(fd, test);
      my_close(fd);
      fd = open_file(pathname, 0);
      char buf[1024];
      memset(buf, 0, 1024);
      my_read(fd, buf, strlen(test) + 1);
    }
    if (!strcmp(cmd, "ps")) {
      ps(pathname);
    }
    if (!strcmp(cmd, "quit"))
    {
      quit();
    }
  }
}
