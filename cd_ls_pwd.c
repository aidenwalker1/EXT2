/************* cd_ls_pwd.c file **************/

#ifndef __CDLSPWD_C__
#define __CDLSPWD_C__

#include "type.h"
#include "util.c"

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

// change directory
int chdir(char *pathname)
{
    int dev; // local in function that calls getino()
    if (pathname[0] == '/')
        dev = root->dev; // absolute pathname
    else
        dev = running->cwd->dev;      // relative pathname
    int ino = getino(pathname, &dev); // pass &dev as an extra parameter
    MINODE *mip = iget(dev, ino);

    // checks is dur
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("%s not a dir\n", pathname);
        return;
    }
    iput(running->cwd);

    // sets cwd to new mip
    running->cwd = mip;
    return 1;
}

// list file info
int ls_file(MINODE *mip, char *name)
{
    INODE node = mip->INODE;
    int mode = node.i_mode;

    // prints out mode
    if ((mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("-");
    if ((mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("d");
    if ((mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("l");
    for (int i = 8; i >= 0; i--)
    {
        if (mode & (1 << i))
        { // print r|w|x
            printf("%c", t1[i]);
        }
        else
        {
            printf("%c", t2[i]);
        }
    }

    char ftime[32];
    memset(ftime, 0, 32);
    long long time = node.i_atime;

    // prints out link count, group id, and size
    printf("  %d  %d  %d  ", node.i_links_count, node.i_gid, node.i_size);

    // gets time
    strcpy(ftime, ctime(&time));
    ftime[strlen(ftime) - 1] = 0;

    // prints time and name
    printf("%s  %s", ftime, name);

    // prints link
    if (S_ISLNK(mip->INODE.i_mode)) {
        char buf[1024];
        memset(buf, 0, 1024);
        my_readlink(name, buf);
        printf(" -> %s", buf);
    }
    printf("\n");
}

// list dir info
int ls_dir(int ino, int dev)
{
    // gets dir info from ino and dev
    char buf[BLKSIZE];
    memset(buf, 0, BLKSIZE);
    DIR *dp;
    get_block(dev, ino, buf);

    char *cp = buf;
    dp = (DIR *)buf;
    char temp[1024];

    // goes through each entry
    while (cp < buf + BLKSIZE)
    {
        // gets name
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;

        // gets inode info
        MINODE *current = iget(dev, dp->inode);
        ls_file(current, temp);
        iput(current);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
}

// ls at pathname
int my_ls(char *pathname)
{
    if (*pathname == 0)
    {
        pathname = ".";
    }

    int dev; // local in function that calls getino()
    if (pathname[0] == '/')
        dev = root->dev; // absolute pathname
    else
        dev = running->cwd->dev;      // relative pathname
    int ino = getino(pathname, &dev); // pass &dev as an extra parameter

    if (ino == 0)
    {
        return;
    }

    MINODE *mip = iget(dev, ino);
    INODE node = mip->INODE;
    int curDev = mip->dev;
    iput(mip);

    // goes through each block
    for (int i = 0; i < node.i_blocks; i++)
    {
        ino = node.i_block[i];

        // no more blocks
        if (ino == 0)
        {
            return;
        }

        // lists dirs at current block
        ls_dir(ino, curDev);
    }
}

// recursive pwd, starts at bottom inode and goes up path
int rpwd(MINODE *wd)
{
    if (wd->ino == 2)
    {
        return;
    }
    // gets current and parent ino
    int parent_ino = -1;
    int my_ino = get_myino(wd, &parent_ino);

    char name[1024];

    // gets to parent node pwds it
    MINODE *pip = iget(wd->dev, parent_ino);
    get_myname(pip, my_ino, name);
    rpwd(pip);
    iput(pip);

    // prints current node name
    printf("%s/", name);
}

// pwd
int my_pwd(MINODE *wd)
{
    if (wd == root)
    {
        printf("/");
    }
    else
    {
        printf(get_mtable(wd->dev)->mntName);
        if (wd->dev != globalDev) {
            printf("/");
        }
        rpwd(wd);
    }

    printf("\n");
}

#endif