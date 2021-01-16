int mount(char *filesystem, char *mountPoint)
{
    MINODE *mip;
    int fd, ino, freeTableIndex = 0;
    char buf[BLKSIZE];
    for (int i = 0; i < NMTABLE; ++i)
    {
        if (mtable[i].dev == 0)
            continue;
        printf("%s\n", mtable[i].mntName);
        if (strcmp(mtable[i].devName, filesystem) == 0)
        {
            printf("File system is already mounted on %s\n", mtable[i].mntName);

            return -1;
        }
    }

    // not mounted
    //allocate new mount table entry

    for (int i = 0; i < NMTABLE; ++i)
    {
        if (mtable[i].dev == 0)
        {
            freeTableIndex = i;
            break;
        }
    }

    fd = open(filesystem, O_RDWR);

    get_block(fd, 1, buf);
    sp = (SUPER *)buf;

    if (sp->s_magic != 0xEF53)
    {
        printf("Not EXT2 filesystem\n");
        return -1;
    }

    ino = getino(mountPoint);
    mip = iget(dev, ino);

    if (S_ISDIR(mip->INODE.i_mode) == 0)
    {
        printf("Not mounting to a directory\n");
        return -1;
    }

    if (mip->refCount > 1)
    {
        printf("Busy\n");
        return -1;
    }

    strcpy(mtable[freeTableIndex].mntName, mountPoint);
    strcpy(mtable[freeTableIndex].devName, filesystem);
    mtable[freeTableIndex].dev = fd;

    mip->mounted = 1;
    mip->mountptr = &mtable[freeTableIndex];

    mtable[freeTableIndex].mntDirPtr = mip;
}

int umount(char *filesystem)
{
    for (int i = 0; i < NMTABLE; ++i)
    {
        if (strcmp(mtable[i].devName, filesystem) == 0)
        {
            printf("Mounted\n");
        }
    }
}