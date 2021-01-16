int read_file()
{
    pfd();

    int fd, numberOfBytes;
    char tempfd[128], tempNumberOfBytes[128];

    printf("Enter FD:\n");
    fgets(tempfd, 128, stdin);

    fd = atoi(tempfd);

    if (running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2)
    {
        printf("File not opened for a read operation\n");
        return -1;
    }

    printf("Enter number of bytes to read:\n");
    fgets(tempNumberOfBytes, 128, stdin);

    numberOfBytes = atoi(tempNumberOfBytes);

    char buffer[numberOfBytes];
    return my_read(fd, buffer, numberOfBytes);
}

int my_read(int fd, char *buf, int numberOfBytes)
{
    MINODE *mip = running->fd[fd]->mptr;
    int count = 0, avil = mip->INODE.i_size - running->fd[fd]->offset;
    int lbk, start, blk, remain, ibuf[256];
    int check = 0;

    char *cq = buf, *cp, readbuf[BLKSIZE];

    while (numberOfBytes && avil)
    {
        //compute logical block num
        lbk = running->fd[fd]->offset / BLKSIZE; // offset increases by num of bytes read
        start = running->fd[fd]->offset % BLKSIZE;

        if (lbk < 12)
        {
            blk = mip->INODE.i_block[lbk];
        }

        else if (lbk >= 12 && lbk < 256 + 12)
        {
            //indirect blocks

            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            blk = ibuf[lbk - 12];
        }

        else // lbk greater than 268
        {
            //double indirect blocks
            // lbk >= 268
            //points to block which points to 256 blocks
            //each points to 256 blocks
            get_block(mip->dev, mip->INODE.i_block[13], ibuf); //have an arry of 256 blocks, and each of those points to 256 disk blocks

            get_block(mip->dev, ibuf[(lbk - (12 + 256)) / 256], ibuf);
            blk = ibuf[(lbk - (12 + 256)) % 256];
        }

        get_block(mip->dev, blk, readbuf);

        cp = readbuf + start;
        remain = BLKSIZE - start;
        int bytesToRead = 0;

        if (numberOfBytes > avil)
        {
            bytesToRead = avil;
        }

        else
        {
            bytesToRead = numberOfBytes;
        }

        strncpy(cq, cp, bytesToRead);
        cq += bytesToRead;
        cp += bytesToRead;

        running->fd[fd]->offset += bytesToRead;
        count += bytesToRead;
        avil -= bytesToRead;
        numberOfBytes -= bytesToRead;
        remain -= bytesToRead;
    }
    return count;
}

int cat(char *filename)
{
    char buf[BLKSIZE];
    int n;
    int fd = open_file(filename, 0);

    while (n = my_read(fd, buf, BLKSIZE))
    {
        buf[n] = 0;
        printf("%s", buf);
    }

    close_file(fd);
}