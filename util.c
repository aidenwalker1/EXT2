/*********** util.c file ****************/

#ifndef __UTIL_C__
#define __UTIL_C__

#include "type.h"

int get_block(int dev, int blk, char *buf)
{
    lseek(dev, blk * BLKSIZE, SEEK_SET);
    int n = read(dev, buf, BLKSIZE);
    if (n < 0)
        printf("get_block[% d % d] error \n", dev, blk);
}

int put_block(int dev, int blk, char *buf)
{
    lseek(dev, blk * BLKSIZE, SEEK_SET);
    int n = write(dev, buf, BLKSIZE);
    if (n != BLKSIZE)
        printf("put_block[% d % d] error \n", dev, blk);
}

// looks for mtable matching dev
MTABLE *get_mtable(int dev)
{
    MTABLE *cur;
    for (int i = 0; i < 10; i++)
    {
        cur = &mtable[i];

        if (cur->dev == dev)
        {
            return cur;
        }
    }

    return NULL;
}

MINODE *mialloc() // allocate a FREE minode for use
{
    int i;
    for (i = 0; i < NMINODE; i++)
    {
        MINODE *mp = &minode[i];
        if (mp->refCount == 0)
        {
            mp->refCount = 1;
            return mp;
        }
    }
    printf("FS panic: out of minodes\n");
    return NULL;
}

MINODE *iget(int dev, int ino)
{
    MINODE *mip;
    MTABLE *mp;
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];

    // serach in-memory minodes first
    for (i = 0; i < NMINODE; i++) ///////////////
    {
        MINODE *mip = &minode[i];
        if (mip->refCount && (mip->dev == dev) && (mip->ino == ino))
        {
            mip->refCount++;
            return mip;
        }
    }
    mp = get_mtable(dev);
    int ib = mp->iblock;
    //ib = iblock;
    // needed INODE=(dev,ino) not in memory
    mip = mialloc(); // allocate a FREE minode
    mip->dev = dev;
    mip->ino = ino;                 // assign to (dev, ino)
    block = (ino - 1) / 8 + ib; // disk block containing this inode
    offset = (ino - 1) % 8;         // which inode in this block
    get_block(dev, block, buf);
    ip = (INODE *)buf + offset;
    mip->INODE = *ip; // copy inode to minode.INODE
    // initialize minode
    mip->refCount = 1;
    mip->dirty = 0;
    mip->mptr = 0;
    return mip;
}

MINODE* mntiget(int dev, int ino, int ib) {
    MINODE *mip;
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];

    // serach in-memory minodes first
    for (i = 0; i < NMINODE; i++) ///////////////
    {
        MINODE *mip = &minode[i];
        if (mip->refCount && (mip->dev == dev) && (mip->ino == ino))
        {
            mip->refCount++;
            return mip;
        }
    }

    // needed INODE=(dev,ino) not in memory
    mip = mialloc(); // allocate a FREE minode
    mip->dev = dev;
    mip->ino = ino;                 // assign to (dev, ino)
    block = (ino - 1) / 8 + ib; // disk block containing this inode
    offset = (ino - 1) % 8;         // which inode in this block
    get_block(dev, block, buf);
    ip = (INODE *)buf + offset;
    mip->INODE = *ip; // copy inode to minode.INODE
    // initialize minode
    mip->refCount = 1;
    mip->dirty = 0;
    mip->mptr = 0;
    return mip;
}

int midalloc(MINODE *mip) // release a used minode
{
    mip->refCount = 0;
}

int iput(MINODE *mip)
{
    INODE *ip;
    MTABLE* mp;
    int i, block, offset;
    char buf[BLKSIZE];
    if (mip == 0)
        return;
    mip->refCount--; // dec refCount by 1
    if (mip->refCount > 0)
        return; // still has user
    if (mip->dirty == 0)
        return; // no need to write back
    // write INODE back to disk
    mp = get_mtable(mip->dev);
    int ib = mp->iblock;
    block = (mip->ino - 1) / 8 + ib;
    offset = (mip->ino - 1) % 8;
    // get block containing this inode
    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + offset;      // ip points at INODE
    *ip = mip->INODE;                // copy INODE to inode in block
    put_block(mip->dev, block, buf); // write back to disk
    midalloc(mip);                   // mip->refCount = 0;
}

int tokenize(char *pathname)
{
    char *s;
    strcpy(gline, pathname);
    nname = 0;
    s = strtok(gline, "/");
    while (s)
    {
        name[nname++] = s;
        s = strtok(0, "/");
    }
}

int search(MINODE *mip, char *name)
{
    int i;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;
    for (i = 0; i < 12; i++)
    { // search DIR direct blocks only
        if (mip->INODE.i_block[i] == 0)
            return 0;
        get_block(mip->dev, mip->INODE.i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        while (cp < sbuf + BLKSIZE)
        {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            if (strcmp(name, temp) == 0)
            {
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

int access(char* filename, char mode) {
    //return 1;
    int r;
    if (running->uid == 0) {
        return 1;
    }
    int dev; // local in function that calls getino()
    if (filename[0] == '/')
        dev = root->dev; // absolute pathname
    else
        dev = running->cwd->dev; // relative pathname
    int ino = getino(filename, &dev);
    MINODE* mip = iget(dev, ino);

    int oR = 0x0100;
    int oW = 0x0080;
    int oX = 0x0040;

    int otherOR = 0x0020;
    int otherOW = 0x0010;
    int otherOX = 0x0001;

    int test;

    if (mode == 'r') {
        test = oR;
    }
    else if (mode == 'w') {
        test = oW;
    }
    else {
        test = oX;
    }
    r = test_intbit(mip->INODE.i_mode, test);

    iput(mip);
    return r;
}

int inoaccess(MINODE* mip, char mode) {
    //return 1;
    int r;
    int oR = 0x0100;
    int oW = 0x0080;
    int oX = 0x0040;

    int otherOR = 0x0020;
    int otherOW = 0x0010;
    int otherOX = 0x0008;

    int test;

    if (mode == 'r') {
        test = oR;
    }
    else if (mode == 'w') {
        test = oW;
    }
    else {
        test = oX;
    }
    r = test_intbit(mip->INODE.i_mode, test);

    return r;
}

int getino(char *pathname, int* dev)
{
    MINODE *mip;
    int i, ino;
    if (strcmp(pathname, "/") == 0)
    {
        return 2; // return root ino=2
    }
    if (pathname[0] == '/') {
        mip = root; // if absolute pathname: start from root
    }
    else {
        mip = running->cwd; // if relative pathname: start from CWD
    }
    mip->refCount++;        // in order to iput(mip) later
    tokenize(pathname);     // assume: name[ ], nname are globals

    for (i = 0; i < nname; i++)
    { // search for each component string
        if (!inoaccess(mip, 'r') || !inoaccess(mip, 'x')) {
            printf("ERROR ATTEMPTING TO ACCESS NON OWNED FILE\n");
            iput(mip);
            return 0;
        }
        if (!S_ISDIR(mip->INODE.i_mode))
        { // check DIR type
            printf("%s is not a directory\n", name[i]);
            iput(mip);
            return 0;
        }

        ino = search(mip, name[i]);

        if (strcmp(name[i], "..") == 0 && ino == 2 && globalDev != *dev) {
            iput(mip);
            *dev = globalDev;
            char newpath[128];
            strcat(newpath, "/");
            while (i+1 < nname) {
                strcat(newpath, name[i]);
                strcat(newpath, "/");
                i++;
            }
            return getino(newpath, &dev);
        }

        if (ino == 0) {
            for (int j = 0; j < 10; j++) {
                MTABLE m = mtable[j];
                if (mtable[j].dev == 0) {
                    break;
                }
                else if (strstr(m.mntName, name[i]))
                {
                    iput(mip);
                    *dev = m.dev;
                    ino = 2;
                    break;
                }
            }
        }
        
        if (!ino)
        {
            printf("no such component name %s\n", name[i]);
            iput(mip);
            return 0;
        }
        iput(mip);                  // release current minode
        mip = iget(*dev, ino); // switch to new minode

        // checks cross mount - not working
        
    }
    iput(mip);
    return ino;
}


// gets parent and current ino
int get_myino(MINODE *mip, int *parent_ino)
{
    INODE node = mip->INODE;

    // gets dir block 1 for cur and parent inos
    int ino = node.i_block[0];
    char buf[1024];
    char *cp;
    get_block(mip->dev, ino, buf);

    // gets cur ino at dir[0], parent at dir[1]
    DIR *dp = (DIR *)buf;
    int my_ino = dp->inode;
    cp = buf;
    cp += dp->rec_len;
    dp = (DIR *)cp;
    *parent_ino = dp->inode;

    return my_ino;
}

// gets name for given ino and parent node
int get_myname(MINODE *parent, int my_ino, char name[1024])
{
    // gets inode
    INODE node = parent->INODE;
    int ino = node.i_block[0];
    char buf[1024];
    char *cp;

    // gets dir block
    get_block(parent->dev, ino, buf);
    cp = buf;

    DIR *dp = (DIR *)buf;

    // skips . and ..
    cp = buf;
    cp += dp->rec_len;
    dp = (DIR *)cp;
    cp += dp->rec_len;
    dp = (DIR *)cp;

    // searches for inode matching the child
    while (cp < buf + BLKSIZE)
    {
        if (my_ino == dp->inode)
        {
            // print child name
            strncpy(name, dp->name, dp->name_len);
            name[dp->name_len] = 0;
            return;
        }

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    return 0;
}



#endif