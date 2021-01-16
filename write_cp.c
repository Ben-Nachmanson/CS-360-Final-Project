
int write_file()
{
    pfd();

    int fd, numberOfBytes, n;
    char tempfd[128], buf[128];

    printf("Enter FD:\n");
    fgets(tempfd, 128, stdin);

    fd = atoi(tempfd);

    if (running->fd[fd] == NULL)
    {
        printf("File not opened for a write operation\n");
        return -1;
    }
    else
    {
        printf("Enter the string you want to write:\n");
        fgets(buf, 128, stdin);

        numberOfBytes = strlen(buf);
        n = my_write(fd, buf, numberOfBytes);
        close_file(fd);
    }

    return n;
}

int my_write(int fd, char *buf, int numberOfBytes)
{
    MINODE *mip = running->fd[fd]->mptr;

    int lbk, blk, start, ibuf[256], doubleIBuf[256], remain, count = 0;
    char *cq = buf, *cp, writebuf[BLKSIZE], tempBuf[BLKSIZE];

    while (numberOfBytes > 0)
    {
        //compute logical block num
        lbk = running->fd[fd]->offset / BLKSIZE; // offset increases by num of bytes read
        start = running->fd[fd]->offset % BLKSIZE;

        if (lbk < 12)
        {
            if (mip->INODE.i_block[lbk] == 0)
            {
                //allocate block
                mip->INODE.i_block[lbk] = balloc(mip->dev);
            }

            blk = mip->INODE.i_block[lbk];
        }

        else if (lbk >= 12 && lbk < 256 + 12)
        {
            if (mip->INODE.i_block[12] == 0)
            {
                blk = mip->INODE.i_block[12] = balloc(mip->dev);

                bzero(tempBuf, BLKSIZE);
                put_block(mip->dev, mip->INODE.i_block[12], tempBuf);
            }

            get_block(mip->dev, mip->INODE.i_block[12], ibuf);

            blk = ibuf[lbk - 12];

            if (blk == 0)
            {
                blk = ibuf[lbk - 12] = balloc(mip->dev);

                bzero(tempBuf, BLKSIZE);
                put_block(mip->dev, blk, tempBuf);
                put_block(mip->dev, mip->INODE.i_block[12], ibuf);
            }
        }

        else
        {
            //Searching for
            if (mip->INODE.i_block[13] == 0)
            {
                mip->INODE.i_block[13] = balloc(mip->dev);

                bzero(tempBuf, BLKSIZE);
                put_block(mip->dev, mip->INODE.i_block[13], tempBuf);
            }

            get_block(mip->dev, mip->INODE.i_block[13], ibuf);

            //have double indirect, 256 blocks, each points to 256 disk blocks
            // block where disk is located = lbk / 256

            if (ibuf[(lbk - (12 + 256)) / 256] == 0)
            {
                ibuf[(lbk - (12 + 256)) / 256] = balloc(mip->dev);

                bzero(tempBuf, BLKSIZE);
                put_block(mip->dev, ibuf[(lbk - (12 + 256)) / 256], tempBuf);

                put_block(mip->dev, mip->INODE.i_block[13], ibuf);
            }
            get_block(mip->dev, ibuf[(lbk - (12 + 256)) / 256], doubleIBuf);

            blk = doubleIBuf[(lbk - (12 + 256)) % 256];

            if (blk == 0)
            {
                blk = doubleIBuf[(lbk - (12 + 256)) % 256] = balloc(mip->dev);
                put_block(mip->dev, ibuf[(lbk - (12 + 256)) / 256], doubleIBuf);
            }
        }

        get_block(mip->dev, blk, writebuf);
        cp = writebuf + start;
        remain = BLKSIZE - start;
        int bytesToWrite = 0;

        if (numberOfBytes > remain)
        {
            bytesToWrite = remain;
        }

        else
        {
            bytesToWrite = numberOfBytes;
        }

        strncpy(cp, cq, bytesToWrite);
        cp += bytesToWrite;
        cq += bytesToWrite;
        numberOfBytes -= bytesToWrite;
        remain -= bytesToWrite;
        count += bytesToWrite;
        running->fd[fd]->offset += bytesToWrite;
        if (running->fd[fd]->offset > mip->INODE.i_size)
            mip->INODE.i_size += bytesToWrite;

        put_block(mip->dev, blk, writebuf);
    }

    mip->dirty = 1;
    printf("wrote %d char into file descriptor fd=%d\n", count, fd);
    return numberOfBytes;
}
int cp(char *src, char *dest)
{
    int fd, gd, n;
    char buf[BLKSIZE];

    printf("SRC: %s, DEST: %s\n", src, dest);

    fd = open_file(src, 0);
    gd = open_file(dest, 2);

    pfd();
    while (n = my_read(fd, buf, BLKSIZE))
    {
        my_write(gd, buf, n);
    }

    close_file(fd);
    close_file(gd);
}

int mv(char *file, char *newFile)
{
    link(file, newFile);

    MINODE *f1, f2;
    int ino = getino(file);
    printf("INO:%d\n", ino);
    ino = getino(newFile);
    printf("INO 2:%d\n", ino);
    unlink(file);
}
