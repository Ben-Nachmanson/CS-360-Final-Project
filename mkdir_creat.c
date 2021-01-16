int enter_name(MINODE *pip, int myino, char *myname)
{
    INODE *tempInode = &pip->INODE;
    char buf[BLKSIZE];
    char *cp;
    DIR *dp;
    int idealLength;
    int neededLength;
    int nameLength = strlen(myname);

    for (int i = 0; i < 12; ++i)
    {
        if (tempInode->i_block[i] == 0)
        {
            break;
        }

        get_block(pip->dev, tempInode->i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        printf("step to LAST entry in data block %d\n", tempInode->i_block[i]);
        while (cp + dp->rec_len < buf + BLKSIZE)
        {
            printf("%s\n", dp->name);

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        printf("current Last entry: %s\n", dp->name);
        cp = (char *)dp;

        idealLength = 4 * ((8 + dp->name_len + 3) / 4);
        neededLength = 4 * ((8 + nameLength + 3) / 4);

        int remain = dp->rec_len - idealLength;
        if (remain >= neededLength)
        { //enough space
            dp->rec_len = idealLength;
            cp += dp->rec_len;
            dp = (DIR *)cp;

            dp->inode = myino;
            dp->rec_len = remain;
            dp->name_len = nameLength;
            strcpy(dp->name, myname);
        }

        else
        {
            //not enough space
            printf("ENTERED ELSE\n");

            int bno = balloc(dev);
            tempInode->i_block[i] = bno;

            pip->INODE.i_size += BLKSIZE;
            pip->dirty = 1;
            get_block(pip->dev, bno, buf);

            dp = (DIR *)buf;
            cp = buf;

            dp->inode = myino;
            strcpy(dp->name, myname);

            dp->rec_len = BLKSIZE;
            dp->name_len = nameLength;

            put_block(pip->dev, bno, buf);
        }
        put_block(pip->dev, tempInode->i_block[i], buf);
    }
}
int mymkdir(MINODE *pip, char *name)
{
    MINODE *mip;

    int ino = ialloc(dev); // allocate inode
    int bno = balloc(dev); //allocate block

    mip = iget(dev, ino);

    INODE *ip = &mip->INODE;
    //initialize directory
    ip->i_mode = 0x41ED;
    ip->i_uid = running->uid;
    ip->i_gid = running->gid;
    ip->i_size = BLKSIZE;
    ip->i_links_count = 2;
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2;
    ip->i_block[0] = bno;
    for (int i = 1; i < 15; ++i)
    {
        ip->i_block[i] = 0;
    }

    mip->dirty = 1;
    iput(mip); // writes ino to the disk

    //write . itself
    char buf[BLKSIZE];
    DIR *dp = (DIR *)buf;
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    //write .. parent
    dp = (char *)dp + 12;
    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE - 12;
    dp->name_len = 2;
    strcpy(dp->name, "..");

    put_block(dev, bno, buf); // writes to block

    enter_name(pip, ino, name); //add new directory to parent
}
int make_dir(char *pathname)
{

    MINODE *pip;
    int pino;

    char fullDirectory[128], newDirectoryName[128], pathCopy[128];
    strcpy(pathCopy, pathname);
    strcpy(fullDirectory, dirname(pathname));
    strcpy(newDirectoryName, basename(pathCopy));

    printf("DIR:%s\n", fullDirectory);
    printf("BASE:%s\n", newDirectoryName);

    if (strcmp(fullDirectory, ".") == 0)
    {
        //relative
        dev = running->cwd->dev;
    }

    else
    {
        //absolute
        dev = root->dev;
    }

    pino = getino(fullDirectory); // get inode num
    pip = iget(dev, pino);
    if (S_ISDIR(pip->INODE.i_mode) == 0)
    {
        printf("%s is not a directory\n", fullDirectory);
        iput(pip);
        return;
    }

    int childIno = search(pip, newDirectoryName);

    if (childIno > 0)
    {
        printf("%s already exists\n", newDirectoryName);
        iput(pip);
        return;
    }

    mymkdir(pip, newDirectoryName);

    pip->INODE.i_links_count += 1;
    pip->dirty = 1;
    pip->INODE.i_atime = pip->INODE.i_ctime = pip->INODE.i_mtime = time(0L);
    iput(pip);
}
int creat_file(char *pathname)
{
    MINODE *pip;
    int pino;

    char fullDirectory[128], newFileName[128], pathCopy[128];
    strcpy(pathCopy, pathname);
    strcpy(fullDirectory, dirname(pathname));
    strcpy(newFileName, basename(pathCopy));

    printf("DIR:%s\n", fullDirectory);
    printf("BASE:%s\n", newFileName);

    if (strcmp(fullDirectory, ".") == 0)
    {
        printf("Relative path\n");
        dev = running->cwd->dev;
    }

    else
    {
        printf("Absolute path\n");
        dev = root->dev;
    }

    pino = getino(fullDirectory);
    pip = iget(dev, pino);

    int childIno = search(pip, newFileName);

    if (childIno > 0)
    {
        MINODE *cip = iget(dev, childIno);
        printf("MODE: %x\n", cip->INODE.i_mode);

        if (S_ISREG(cip->INODE.i_mode) == 0)
        {
            printf("%s is not a file; it is a directory\n", newFileName);
        }

        else
        {
            printf("%s already exists\n", newFileName);
        }

        iput(cip);
        iput(pip);
        return;
    }

    my_creat(pip, newFileName);

    pip->dirty = 1;
    pip->INODE.i_atime = pip->INODE.i_ctime = pip->INODE.i_mtime = time(0L);
    iput(pip);
}
int my_creat(MINODE *pip, char *name)
{
    MINODE *mip;

    int ino = ialloc(dev);
    mip = iget(dev, ino);

    INODE *ip = &mip->INODE;
    ip->i_mode = 0x81A4;
    ip->i_uid = running->uid;
    ip->i_gid = running->gid;
    ip->i_size = 0;
    ip->i_links_count = 1;
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 0;
    for (int i = 0; i < 15; ++i)
    {
        ip->i_block[i] = 0;
    }

    mip->dirty = 1;
    iput(mip);

    enter_name(pip, ino, name);
}
