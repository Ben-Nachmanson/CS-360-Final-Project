int link(char *oldFile, char *newFile)
{
    MINODE *omip, *pmip;
    int oino = getino(oldFile), pino;

    char directoryName[128], fileName[128], newFileCopy[128];

    if (oino <= 0)
    {
        printf("File %s doesn't exist\n", oldFile);
        return;
    }

    omip = iget(dev, oino);

    if (S_ISDIR(omip->INODE.i_mode))
    {
        printf("%s is not a file, it is a directory\n", oldFile);
        iput(omip);
        return;
    }

    if (getino(newFile) > 0)
    {
        printf("%s already exists\n");
        iput(omip);
        return;
    }

    strcpy(newFileCopy, newFile);
    strcpy(directoryName, dirname(newFile));
    strcpy(fileName, basename(newFileCopy));

    pino = getino(directoryName);
    pmip = iget(dev, pino);

    enter_name(pmip, oino, fileName);
    omip->INODE.i_links_count++;
    omip->dirty = 1;
    iput(omip);
    iput(pmip);
}

int unlink(char *filename)
{
    MINODE *mip, *pmip;
    int ino, pino;

    char directoryName[128], fileName[128], fileNameCopy[128];

    ino = getino(filename);

    if (ino <= 0)
    {
        printf("File doesn't exist\n");
        return -1;
    }
    mip = iget(dev, ino);

    if (S_ISLNK(mip->INODE.i_mode) || S_ISREG(mip->INODE.i_mode))
    {
        strcpy(fileNameCopy, filename);
        strcpy(directoryName, dirname(filename));
        strcpy(fileName, basename(fileNameCopy));

        pino = getino(directoryName);
        pmip = iget(dev, pino);

        mip->INODE.i_links_count--;

        if (mip->INODE.i_links_count > 0)
        {
            mip->dirty = 1; // mark dirty if we need to write to it
        }

        else
        {
            for (int i = 0; i < 12; ++i)
            {
                if (mip->INODE.i_block[i] == 0)
                {
                    continue;
                }
                bdalloc(mip->dev, mip->INODE.i_block[i]);
            }

            idalloc(mip->dev, ino);
        }

        iput(mip);
    }

    else
    {
        printf("%s is not a file or link\n", fileName);
        iput(mip);
        return;
    }

    rm_child(pmip, fileName);

    iput(pmip);
}
