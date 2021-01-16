int open_file(char *file, int mode)
{
    MINODE *mip;
    OFT openFile;
    int ino, openFilesIndex, fd;

    if (file[0] == '/')
    {
        printf("Absolute path\n");
        dev = root->dev;
    }

    else
    {
        printf("Relative path\n");
        dev = running->cwd->dev;
    }

    ino = getino(file);

    if (ino == 0)
    {
        creat_file(file);
        ino = getino(file);
    }

    mip = iget(dev, ino);

    if (S_ISREG(mip->INODE.i_mode) == 0)
    {
        printf("Not a regular file\n");
        iput(mip);
        return -1;
    }

    // check if file is already opened

    for (int i = 0; i < NOFT; ++i)
    {
        if (openFiles[i].refCount == 0)
        {
            openFilesIndex = i;
            break;
        }
    }

    openFile.mode = mode;
    openFile.mptr = mip;
    openFile.refCount = 1;
    //which mode

    if (mode == 0 || mode == 2)
    {
        openFile.offset = 0;
    }

    else if (mode == 1)
    {
        truncate(mip);
        openFile.offset = 0;
    }

    else
    {
        openFile.offset = mip->INODE.i_size;
    }

    openFiles[openFilesIndex] = openFile;
    for (int i = 0; i < NFD; ++i)
    {
        if (running->fd[i] == NULL)
        {
            running->fd[i] = &openFiles[openFilesIndex]; //open files array
            fd = i;
            break;
        }
    }

    if (mode == 0)
    {
        mip->INODE.i_atime = time(0L);
    }

    else
    {
        mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
    }

    mip->dirty = 1;
    return fd;
}

int open_userInput(char *file)
{
    char tempmode[128];
    int mode, fd;

    printf("Enter the open mode (O|1|2|3 for R|W|RW|APPEND)\n");
    fgets(tempmode, 128, stdin);

    mode = atoi(tempmode);

    printf("Mode: %d\n", mode);

    if (mode < 0 || mode > 3)
    {
        printf("Invalid mode\n");
        return -1;
    }

    fd = open_file(file, mode);

    if (fd >= 0)
    {
        printf("FD: %d\n", fd);
    }

    return 0;
}

int truncate(MINODE *mip)
{
    for (int i = 0; i < 15; i++)
    {
        if (mip->INODE.i_block[i] == 0)
            continue;
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
    mip->INODE.i_ctime = time(0L);
    mip->INODE.i_dtime = time(0L);
    mip->INODE.i_size = 0;
    mip->dirty = 1;
    return;
}

int close_file(int fd)
{
    if (running->fd[fd] != NULL)
    {
        running->fd[fd]->refCount--;
        if (running->fd[fd]->refCount == 0) // not being used
        {
            iput(running->fd[fd]->mptr); // deletes
        }
        running->fd[fd] = 0;

        // for (int i = 0; i < NOFT; ++i)
        // {
        //     // if (openFiles[i].fd == fd)
        //     // {
        //     // }
        // }
    }
    else
    {
        printf("File isn't open\n");
    }
}

int pfd()
{
    printf("Open Files: \n");
    for (int i = 0; i < NFD; ++i)
    {
        if (running->fd[i] != NULL)
        {
            printf("FD:%d MODE:%d OFFSET:%d\n", i, running->fd[i]->mode, running->fd[i]->offset);
        }
    }
}

int close_userInput()
{
    char tempfd[128];
    int fd;
    pfd();

    printf("Enter the fd of the file you want to close\n");
    fgets(tempfd, 128, stdin);

    fd = atoi(tempfd);

    if (fd > NFD || fd < 0)
    {
        printf("Out of range\n");
        return -1;
    }

    close_file(fd);
}

int my_lseek()
{
    int fd, position;
    char tempfd[128], tempPosition[128];
    printf("Enter fd\n");
    fgets(tempfd, 128, stdin);
    tempfd[strlen(tempfd) - 1] = 0;

    printf("Enter position\n");
    fgets(tempPosition, 128, stdin);
    tempPosition[strlen(tempPosition) - 1] = 0;

    fd = atoi(tempfd);
    position = atoi(tempPosition);

    if (position > running->fd[fd]->mptr->INODE.i_size || position < 0)
        return;
    running->fd[fd]->offset = position;
}