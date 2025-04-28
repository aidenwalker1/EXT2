/************* rmdir.c file **************/

#ifndef __RMDIR_C__
#define __RMDIR_C__

#include "open_close.c"

// removes child from dir
int rm_child(MINODE *pip, char *name)
{
    char buf[1024];
    DIR *dp;
    char *cp;

    for (int i = 0; i < 12; i++)
    {
        if (pip->INODE.i_block[i] == 0)
        {
            break;
        }
        
        // gets block data
        get_block(pip->dev, pip->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        int index = 0;
        cp = buf;
        DIR *last = NULL;

        int found = 0;
        // goes until entry found
        while (cp + dp->rec_len < buf + BLKSIZE)
        {
            //compares name
            if (dp->name_len == strlen(name) && memcmp(dp->name, name, strlen(name)) == 0)
            {
                found = 1;
                break;
            }
            last = dp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
            index++;
        }
        //compares name
        if (dp->name_len == strlen(name) && memcmp(dp->name, name, strlen(name)) == 0)
        {
            found = 1;
        }
        // checks if block contains entry
        if (!found) {
            continue;
        }

        // checks remove case
        if (index == 2 && !(cp + dp->rec_len < buf + BLKSIZE))
        { // first and only
            last->rec_len += dp->rec_len;
            index = BLKSIZE - dp->rec_len;

            // removes everything after index
            while (index < 1024)
            {
                buf[index] = 0;
                index++;
            }
        }
        else if (!(cp + dp->rec_len < buf + BLKSIZE))
        { // last in dir
            // sets last node rec len
            last->rec_len += dp->rec_len;
            index = BLKSIZE - last->rec_len;
            int new_index = BLKSIZE - dp->rec_len;

            // puts new last node into data
            memcpy(buf + index, last, sizeof(DIR));

            index = new_index;
            while (memcmp(buf + index, name, strlen(name)) != 0)
            {
                index++;
            }

            index += strlen(name);

            // clears name junk
            while (index < 1024)
            {
                buf[index] = 0;
                index++;
            }
        }
        else
        { // has nodes after
            char newbuf[1024];
            memcpy(newbuf, buf, 1024);

            int old_rec = dp->rec_len;
            index = cp - buf;

            cp += dp->rec_len;
            dp = (DIR *)cp;

            // runs until last entry
            while (cp < buf + BLKSIZE)
            {
                if (!(cp + dp->rec_len < buf + BLKSIZE))
                {
                    dp->rec_len += old_rec;
                }

                // copies last dir into current dir
                memcpy(newbuf + index, dp, sizeof(DIR));
                index += dp->rec_len;

                // goes to next dir
                cp += dp->rec_len;
                dp = (DIR *)cp;
            }

            while (memcmp(buf + index, name, strlen(name)) != 0)
            {
                index++;
            }

            index += strlen(name);

            // last name junk
            while (index < 1024)
            {
                buf[index] = 0;
                index++;
            }

            put_block(pip->dev, pip->INODE.i_block[i], newbuf);
            return;
        }

        put_block(pip->dev, pip->INODE.i_block[i], buf);
        break;
    }
}

// removes directory
int remove_dir(char *pathname)
{
    int dev;
    if (pathname[0] == '/')
        dev = root->dev; // absolute pathname
    else
        dev = running->cwd->dev;
    int ino = getino(pathname, &dev);
    
    // makes sure pathname exists
    if (ino == 0)
    {
        printf("%s doesnt exist\n", pathname);
        return;
    }

    MINODE *mip = iget(dev, ino);

    // makes sure path is dir
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("%s is not a dir\n", pathname);
        return;
    }

    if (mip->INODE.i_uid != running->uid) {
        printf("not owner\n");
        return;
    }

    // gets parent
    int parent_ino = -1;
    get_myino(mip, &parent_ino);
    MINODE *pmip = iget(mip->dev, parent_ino);

    char buf[1024];
    get_block(mip->dev, mip->INODE.i_block[0], buf);
    DIR* dp = (DIR*)buf;
    char* cp = buf + dp->rec_len;
    dp = (DIR*)cp;

    // makes sure dir isnt empty
    if (cp +dp->rec_len < buf + BLKSIZE)
    {
        printf("%s not empty\n", pathname);
        return;
    }

    char name[1024];
    memset(name, 0, 1024);
    get_myname(pmip, ino, name);
    
    // removes child in parent dir
    rm_child(pmip, name);

    // decrements count
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;
    iput(pmip);

    // deallocates blocks
    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev, mip->ino);

    iput(mip);
    return 1;
}

#endif