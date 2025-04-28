/************* alloc_dealloc.c file **************/

#ifndef __ALLOC_DEALLOC_C__
#define __ALLOC_DEALLOC_C__

#include "util.c"

// tst_bit, set_bit functions
int tst_bit(char *buf, int bit)
{
    return buf[bit / 8] & (1 << (bit % 8));
}

int test_intbit(int test, int bit) {
    return ((test & ( 1 << 8 )) >> 8) & (1 << (bit % 8));
}

int set_bit(char *buf, int bit)
{
    buf[bit / 8] |= (1 << (bit % 8));
}

int clr_bit(char *buf, int bit) // clear bit in char buf[BLKSIZE]
{
    buf[bit / 8] &= ~(1 << (bit % 8));
}

int decFreeInodes(int dev)
{
    char buf[BLKSIZE];
    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
{
    char buf[BLKSIZE];
    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}

int balloc(int dev)
{
    int i;
    char buf[BLKSIZE];
    memset(buf, 0, BLKSIZE);
    // use imap, ninodes in mount table of dev
    MTABLE *mp = (MTABLE *)get_mtable(dev);

    get_block(dev, mp->bmap, buf);
    for (i = 0; i < mp->nblocks; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            put_block(dev, mp->bmap, buf);
            // update free block count in SUPER and GD
            decFreeBlocks(dev);
            return (i + 1);
        }
    }
    return 0; // out of FREE inodes
}

int ialloc(int dev)
{
    int i;
    char buf[BLKSIZE];
    // use imap, ninodes in mount table of dev
    MTABLE *mp = (MTABLE *)get_mtable(dev);
    get_block(dev, mp->imap, buf);

    for (i = 0; i < mp->ninodes; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            put_block(dev, mp->imap, buf);
            // update free inode count in SUPER and GD
            decFreeInodes(dev);
            return (i + 1);
        }
    }
    return 0; // out of FREE blocks
}

int incFreeInodes(int dev)
{
    char buf[BLKSIZE];
    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

int incFreeBlocks(int dev)
{
    char buf[BLKSIZE];
    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_blocks_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}

int idalloc(int dev, int ino)
{
    int i;
    char buf[BLKSIZE];
    MTABLE *mp = (MTABLE *)get_mtable(dev);
    if (ino > mp->ninodes)
    { // niodes global
        printf("inumber %d out of range\n", ino);
        return;
    }
    // get inode bitmap block
    get_block(dev, mp->imap, buf);
    clr_bit(buf, ino - 1);
    // write buf back
    put_block(dev, mp->imap, buf);
    // update free inode count in SUPER and GD
    incFreeInodes(dev);
}

int bdalloc(int dev, int ino)
{
    int i;
    char buf[BLKSIZE];
    MTABLE *mp = (MTABLE *)get_mtable(dev);
    if (ino > mp->ninodes)
    { // niodes global
        printf("inumber %d out of range\n", ino);
        return;
    }
    // get block bitmap block
    get_block(dev, mp->bmap, buf);
    clr_bit(buf, ino - 1);
    // write buf back
    put_block(dev, mp->bmap, buf);
    // update free block count in SUPER and GD
    incFreeBlocks(dev);
}

#endif