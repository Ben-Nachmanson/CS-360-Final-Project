int symlink(char *newName, char *oldName)
{
    int oino = getino(oldName);

    if (oino <= 0)
    {
        printf("%s doesn't exist\n", oldName);
        return;
    }

    if (getino(newName) > 0)
    {
        printf("%s already exists\n", newName);
        return;
    }

    creat_file(newName);
    MINODE *mip;
    int newIno;
    newIno = getino(newName);
    mip = iget(dev, newIno);

    mip->INODE.i_mode = 0xA1FF;
    mip->INODE.i_size = strlen(oldName);
    mip->dirty = 1;
    strcpy((char *)mip->INODE.i_block, oldName);

    iput(mip);
}

int readlink(char *file, char *buffer)
{
    MINODE *mip;
    int ino;

    ino = getino(file);

    mip = iget(dev, ino);
    if (S_ISLNK(mip->INODE.i_mode))
    {
        strcpy(buffer, (char *)mip->INODE.i_block);
        iput(mip);
        return mip->INODE.i_size;
    }
    iput(mip);
    return;
}
