int rmdir(char *pathname)
{
    MINODE *mip;
    DIR *dp;
    char buf[BLKSIZE], *cp;
    int ino;
    int entriesInDirectory = 0;

    if (pathname[0] == '/')
    {
        printf("Absolute directory\n");
        dev = root->dev;
    }

    else
    {
        printf("Relative directory\n");
        dev = running->cwd->dev;
    }

    ino = getino(pathname);
    mip = iget(dev, ino);

    if (S_ISDIR(mip->INODE.i_mode) == 0)
    {
        printf("%s is not a directory\n", pathname);
        iput(mip);
        return;
    }

    if (mip->refCount > 1)
    {
        printf("%s is busy\n", pathname);
        iput(mip);
        return;
    }

    //check if directory is empty
    for (int i = 0; i < 12; i++) //p305
    {
        if (mip->INODE.i_block[i] == 0)
            break;
        get_block(mip->dev, mip->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;
        while (cp < buf + BLKSIZE)
        {
            entriesInDirectory += 1;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }

    if (entriesInDirectory > 2)
    {
        printf("%s is not an empty directory\n", pathname);
        iput(mip);
        return;
    }

    MINODE *pmip;
    int pino;
    char dirName[128];

    pino = findino(mip, &ino);
    pmip = iget(mip->dev, pino);

    findmyname(pmip, ino, dirName);

    rm_child(pmip, dirName);

    pmip->dirty = 1;
    pmip->INODE.i_links_count -= 1;
    iput(pmip);

    for (int i = 0; i < 12; ++i)
    {
        if (mip->INODE.i_block[i] == 0)
        {
            continue;
        }
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }

    idalloc(mip->dev, mip->ino);

    iput(mip);
}

int rm_child(MINODE *parent, char *name)
{
    DIR *dp, *secondToLastEntry;
    char *foundDirectory, *nextEntry, *cp, buf[BLKSIZE];

    int dirRecLength, recLengsBeforeDir = 0, passedDir = 0;

    for (int i = 0; i < 12; ++i)
    {
        if (parent->INODE.i_block[i] == 0)
        {
            break;
        }

        get_block(parent->dev, parent->INODE.i_block[i], buf);

        dp = (DIR *)buf;
        cp = buf;

        while (cp + dp->rec_len < buf + BLKSIZE)
        {
            printf("%s\n", dp->name);

            if (strncmp(dp->name, name, dp->name_len) == 0)
            {
                printf("Found dir\n");
                foundDirectory = cp;
                dirRecLength = dp->rec_len;
                nextEntry = cp + dp->rec_len;
                passedDir = 1;
            }

            else if (passedDir == 0)
            {
                recLengsBeforeDir += dp->rec_len;
            }

            secondToLastEntry = dp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        if (strncmp(dp->name, name, dp->name_len) == 0)
        {
            printf("Last entry\n");
            secondToLastEntry->rec_len += dp->rec_len;

            put_block(parent->dev, parent->INODE.i_block[i], buf);
            return;
        }

        else
        {
            printf("Not last entry\n");
            dp->rec_len += dirRecLength;
            int size = recLengsBeforeDir + dirRecLength;
            size = BLKSIZE - size;

            memcpy(foundDirectory, nextEntry, size);
            put_block(parent->dev, parent->INODE.i_block[i], buf);

            return;
        }
    }
}
