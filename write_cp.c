/************* write_cp.c file **************/

#ifndef __WRITECP_C__
#define __WRITECP_C__

#include "read_cat.c"
#include "link_unlink.c"

// writes buf to fd
int write_file(int fd, char *buf)
{
    int len = strlen(buf) + 1;
    return my_write(fd, buf, len); // Eventually: return the results of my_write
}

// writes buf to fd n times
int my_write(int fd, char buf[], int nbytes)
{
    int count = 0;

    // makes sure fd in bounds
    if (fd > -1 && fd < 16)
    {
        // makes sure fd is open and mode is w
        if (running->fd[fd] != 0 && running->fd[fd]->mode != 0)
        {
            // gets oft data
            OFT *op = running->fd[fd];
            MINODE *mip = op->minodeptr;

            // runs while want more bytes
            while (nbytes)
            {
                // gets block
                int lbk = op->offset / BLKSIZE;
                int start = op->offset % BLKSIZE;
                int blk = map(lbk, mip);

                // gets block data
                char kbuf[1024];
                memset(kbuf, 0, 1024);
                get_block(mip->dev, blk, kbuf);

                char *cp = kbuf + start;
                int remain = BLKSIZE - start;

                // find min of remain, nbytes
                int min = remain;

                if (min > nbytes)
                {
                    min = nbytes;
                }

                // copies data to cp buf
                memcpy(cp, buf, min);

                // updates offsets
                op->offset += min;
                buf += min;
                cp += min;
                count += min;
                nbytes -= min;

                if (op->offset > mip->INODE.i_size)
                {
                    mip->INODE.i_size += min;
                }
                else
                {
                    mip->INODE.i_size = op->offset;
                }

                // writes buf to disk
                put_block(mip->dev, blk, kbuf);
            }
            mip->dirty = 1;
        }
    }

    return count; // Eventually: return the number of bytes written
}

// copies file1 to file2
int my_cp(char *pathname1, char *pathname2)
{
    int dev1;
    if (pathname1[0] == '/')
        dev1 = root->dev; // absolute pathname
    else
        dev1 = running->cwd->dev;
    int ino1 = getino(pathname1, &dev1);

    // checks path1 exists
    if (ino1 == 0)
    {
        printf("%s doesnt exist\n", pathname1);
        return;
    }
    int dev2;
    if (pathname2[0] == '/')
        dev2 = root->dev; // absolute pathname
    else
        dev2 = running->cwd->dev;
    int ino2 = getino(pathname2, &dev2);

    // checks path2 doesnt exist
    if (ino2 != 0)
    {
        printf("%s exists already\n", pathname2);
        return;
    }

    // opens both files
    int fd1 = open_file(pathname1, 0);
    int fd2 = open_file(pathname2, 1);
    MINODE *mip = iget(dev1, ino1);

    char buf[1024];
    int rd = -1;

    // run while stuff to read
    while (rd != 0)
    {
        // resets buf and reads
        memset(buf, 0, 1024);
        rd = my_read(fd1, buf, 1024);

        // writes buf
        my_write(fd2, buf, rd);
    }

    // closes both files
    my_close(fd1);
    my_close(fd2);

    iput(mip);
    return 1;
}

#endif