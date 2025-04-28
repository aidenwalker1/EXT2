/************* link_unlink.c file **************/

#ifndef __LINKUNLINK_C__
#define __LINKUNLINK_C__

#include "mkdir_creat.c"
#include "rmdir.c"

int my_link(MINODE *filePip, char *fileName, int fileIno, MINODE *linkPip, char *linkName)
{
    return 1;
}

// links newpath to oldpath
int link_file(char *oldpath, char *newpath)
{
    int odev;
    if (oldpath[0] == '/')
        odev = root->dev;
    else
        odev = running->cwd->dev;
    // gets old ino
    int oino = getino(oldpath, &odev);

    // checks ino exists
    if (oino == 0)
    {
        printf("%s doesnt exist", oldpath);
        return;
    }

    int dev;
    if (oldpath[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;
    // gets new ino
    int nino = getino(newpath, &dev);

    // check ino doesnt exist
    if (nino != 0)
    {
        printf("%s already exists", newpath);
    }

    // gets mip of old file
    MINODE *omip = iget(odev, oino);

    // checks is a dir
    if (S_ISDIR(omip->INODE.i_mode))
    {
        printf("%s is a dir\n", oldpath);
        iput(omip);
        return;
    }

    if (!access(oldpath, 'w')) {
        printf("no w access\n");
        iput(omip);
        return;
    }

    // gets base and dir name
    strcpy(dname, newpath);
    strcpy(bname, newpath);
    char *parent = dirname(dname);
    char *child = basename(bname);

    if (oldpath[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    // gets parent ino
    int pino = getino(parent, &dev);

    // enters name into parent
    MINODE *pmip = iget(dev, pino);

    if (!access(newpath, 'w')) {
        printf("no w access\n");
        iput(pmip);
        return;
    }

    enter_name(pmip, oino, child);

    // increments link count
    omip->INODE.i_links_count++;
    omip->dirty = 1;
    iput(pmip);
    iput(omip);
}

// unlinks path
int my_unlink(char *pathname)
{
    int dev; // local in function that calls getino()
    if (pathname[0] == '/')
        dev = root->dev; // absolute pathname
    else
        dev = running->cwd->dev;      // relative pathname
    int ino = getino(pathname, &dev); // pass &dev as an extra parameter

    // checks path exists
    if (ino == 0)
    {
        printf("%s doesnt exist", pathname);
        return;
    }

    MINODE *mip = iget(dev, ino);

    // checks path is dir
    if (S_ISDIR(mip->INODE.i_mode))
    {
        printf("%s is a dir\n", pathname);
        iput(mip);
        return;
    }

    if (mip->INODE.i_uid != running->uid) {
        printf("not owner\n");
        return;
    }

    // gets base and dir names
    strcpy(dname, pathname);
    strcpy(bname, pathname);
    char *parent = dirname(dname);
    char *child = basename(bname);

    if (pathname[0] == '/')
        dev = root->dev; // absolute pathname
    else
        dev = running->cwd->dev;     // relative pathname
    int pino = getino(parent, &dev); // pass &dev as an extra parameter

    // gets parent dir
    MINODE *pmip = iget(dev, pino);

    // removes pathname from parent, decrements link count
    rm_child(pmip, child);
    pmip->dirty = 1;
    iput(pmip);

    mip->INODE.i_links_count--;

    // checks if no more links
    if (mip->INODE.i_links_count > 0)
    {
        mip->dirty = 1;
    }
    else
    {
        for (int i = 0; i < 15; i++)
        {
            if (mip->INODE.i_block[i] == 0)
            {
                break;
            }

            // clears out mip block
            char buf[1024];
            memset(buf, 0, 1024);
            put_block(mip->dev, mip->INODE.i_block[i], buf);
            mip->INODE.i_block[i] = 0;
        }
    }
    iput(mip);
    return 1;
}

int my_rm(MINODE *mip, char *pathname)
{
    return 1;
}

#endif