/************* mkdir_creat.c file **************/

#ifndef __MKDIRCREAT_C__
#define __MKDIRCREAT_C__

#include "type.h"
#include "util.c"
#include "alloc_dealloc.c"

// enters name into pip dir
int enter_name(MINODE *pip, int ino, char *name)
{
    char buf[1024];
    DIR *dp;
    char *cp;
    for (int i = 0; i < 12; i++)
    {
        // if need to allocate new block
        if (pip->INODE.i_block[i] == 0)
        {
            // allocates new block
            int blk = balloc(pip->dev);
            pip->INODE.i_block[i] = blk;
        }

        // gets current block
        get_block(pip->dev, pip->INODE.i_block[i], buf);
        dp = (DIR *)buf;

        // goes to last entry
        cp = buf;
        while (cp + dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        int ideal_length = 4 * ((8 + dp->name_len + 3) / 4);
        int need_length = 4 * ((8 + strlen(name) + 3) / 4);
        int remain = dp->rec_len - ideal_length;

        // checks dir full
        if (remain >= need_length)
        {
            // sets last dp length to its ideal lengt
            dp->rec_len = ideal_length;

            // saves into buf
            memcpy(cp, dp, sizeof(DIR));
            while (memcmp(cp, dp->name, strlen(dp->name)) != 0)
            {
                cp++;
            }

            cp += dp->name_len;

            // clears junk name
            while (cp < buf + 1024)
            {
                *cp = 0;
                cp++;
            }

            // goes to new dp entry, saves data
            dp = dp + ideal_length;
            dp->inode = ino;
            dp->rec_len = remain; // rec_len spans block
            dp->name_len = strlen(name);
            strcpy(dp->name, name);
        }
        else
        {
            continue;
        }

        int index = BLKSIZE - remain;

        // copies new dir entry into buf
        memcpy(buf + index, dp, sizeof(DIR));
        while (memcmp(buf + index, dp->name, strlen(dp->name)) != 0)
        {
            index++;
        }

        index += dp->name_len;

        // clears junk name after input name
        while (index < 1024)
        {
            buf[index] = 0;
            index++;
        }

        // saves buf
        put_block(pip->dev, pip->INODE.i_block[i], buf);
        break;
    }
}

// same as kmkdir
int my_mkdir(MINODE *pmip, char *base)
{
    // allocates ino and block
    int ino = ialloc(pmip->dev);
    int blk = balloc(pmip->dev);

    MINODE *mip = iget(pmip->dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED;      // 040755: DIR type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = BLKSIZE;     // size in bytes
    ip->i_links_count = 2;    // links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2;     // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk; // new DIR has one data block

    for (int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;
    }

    mip->dirty = 1; // mark minode dirty
    iput(mip);      // write INODE to disk

    char buf[BLKSIZE];
    bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
    DIR *dp = (DIR *)buf;
    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    // make .. entry: pino=parent DIR ino, blk=allocated block
    dp = (char *)dp + 12;
    dp->inode = pmip->ino;
    dp->rec_len = BLKSIZE - 12; // rec_len spans block
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(pmip->dev, blk, buf); // write to blk on diks

    // enters name into parent dir
    enter_name(pmip, ino, base);
}

// creates dir from foldername
int make_dir(char *foldername)
{
    // gets dir and base names
    strcpy(dname, foldername);
    strcpy(bname, foldername);
    char *dir = dirname(dname);
    char *base = basename(bname);

    int dev; // local in function that calls getino()
    if (foldername[0] == '/')
        dev = root->dev; // absolute pathname
    else
        dev = running->cwd->dev; // relative pathname
    int pino = getino(dir, &dev);

    // checks parent exists
    if (pino == 0)
    {
        printf("%s doesnt exist\n", dir);
        return;
    }

    MINODE *pmip = iget(dev, pino);

    // makes sure parent is dir
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("%s not a dir\n", dir);
        iput(pmip);
        return;
    }

    // makes sure base doesnt exist
    if (search(pmip, base) != 0)
    {
        printf("%s already exists\n", base);
        iput(pmip);
        return;
    }

    if (!access(base, 'w')) {
        printf("need w permission\n");
        iput(pmip);
        return;
    }

    // puts dir into parent
    my_mkdir(pmip, base);

    // increments link count
    pmip->INODE.i_links_count++;
    pmip->dirty = 1;
    iput(pmip);
}

// creates file with kmkdir
int my_creat(MINODE *pmip, char *base)
{
    // gets ino and block data
    int ino = ialloc(pmip->dev);
    int blk = balloc(pmip->dev);

    MINODE *mip = iget(pmip->dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 33188;       // 040755: DIR type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = 0;           // size in bytes
    ip->i_links_count = 1;    // links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2;     // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk; // new DIR has one data block

    for (int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;
    }

    mip->dirty = 1; // mark minode dirty
    iput(mip);      // write INODE to disk

    char buf[BLKSIZE];
    bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
    DIR *dp = (DIR *)buf;
    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    // make .. entry: pino=parent DIR ino, blk=allocated block
    dp = (char *)dp + 12;
    dp->inode = pmip->ino;
    dp->rec_len = BLKSIZE - 12; // rec_len spans block
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(pmip->dev, blk, buf); // write to blk on diks

    // enters entry into parent dir
    enter_name(pmip, ino, base);
}

// creates file
int creat_file(char *pathname)
{
    // gets dir and base names
    strcpy(dname, pathname);
    strcpy(bname, pathname);
    char *dir = dirname(dname);
    char *base = basename(bname);

    int dev; // local in function that calls getino()
    if (pathname[0] == '/')
        dev = root->dev; // absolute pathname
    else
        dev = running->cwd->dev; // relative pathname
    int pino = getino(dir, &dev);

    // makes sure dir exists
    if (pino == 0)
    {
        printf("%s doesnt exist", dir);
        return;
    }

    MINODE *pmip = iget(dev, pino);

    // makes sure parent is dir
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("%s is a dir\n", dir);
        iput(pmip);
        return;
    }

    // makes sure base doesnt exist
    if (search(pmip, base) != 0)
    {
        printf("%s already exists\n", base);
        iput(pmip);
        return;
    }

    if (!access(base, 'w')) {
        printf("need w permission\n");
        iput(pmip);
        return;
    }

    // puts new file into parent dir
    my_creat(pmip, base);

    pmip->dirty = 1;
    iput(pmip);
}

#endif