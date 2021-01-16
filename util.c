/*********** util.c file ****************/

int get_block(int dev, int blk, char *buf)
{
  lseek(dev, (long)blk * BLKSIZE, 0);
  int n = read(dev, buf, BLKSIZE);
  if (n < 0)
    printf("get_block [%d %d] error\n", dev, blk);
}
int put_block(int dev, int blk, char *buf)
{
  lseek(dev, (long)blk * BLKSIZE, 0);
  int n = write(dev, buf, BLKSIZE);
  if (n < 0)
    printf("put_block [%d %d] error\n", dev, blk);
}

MINODE *mialloc()
{
  int i;
  for (i = 0; i < NMINODE; i++)
  {
    MINODE *mp = &minode[i];
    if (mp->refCount == 0)
    {
      mp->refCount = 1;
      return mp;
    }
  }

  printf("FS panic: out of minodes\n");
  return 0;
}

int midalloc(MINODE *mip)
{
  mip->refCount = 0;
}

int nname;

int tokenize(char *pathname)
{
  // copy pathname into gpath[]; tokenize it into name[0] to name[n-1]
  // Code in Chapter 11.7.2

  char *s;
  strcpy(gpath, pathname);
  nname = 0;
  s = strtok(gpath, "/");
  while (s)
  {
    name[nname++] = s;
    s = strtok(0, "/");
  }
}

MINODE *iget(int dev, int ino)
{
  // return minode pointer of loaded INODE=(dev, ino)
  // Code in Chapter 11.7.2

  MINODE *mip;
  MTABLE *mp;
  INODE *ip;
  int i, block, offset;
  char buf[BLKSIZE];

  //search in memory minodes first
  for (i = 0; i < NMINODE; i++)
  {
    MINODE *mip = &minode[i];
    if (mip->refCount && (mip->dev == dev) && (mip->ino == ino))
    {
      mip->refCount++;
      return mip;
    }
  }

  //needed INODE=(dev,ino) not in memory
  mip = mialloc();
  mip->dev = dev;
  mip->ino = ino;
  block = (ino - 1) / 8 + iblock;
  offset = (ino - 1) % 8;
  get_block(dev, block, buf);
  ip = (INODE *)buf + offset;
  mip->INODE = *ip;
  //initialize minode
  mip->refCount = 1;
  mip->mounted = 0;
  mip->dirty = 0;
  mip->mountptr = 0;
  return mip;
}

void iput(MINODE *mip)
{
  // dispose of minode pointed by mip
  // Code in Chapter 11.7.2
  INODE *ip;
  int i, block, offset;
  char buf[BLKSIZE];

  if (mip == 0)
    return;
  mip->refCount--;
  if (mip->refCount > 0)
    return;
  if (mip->dirty == 0)
    return;

  //write INODE back to disk
  block = (mip->ino - 1) / 8 + iblock;
  offset = (mip->ino - 1) % 8;

  //get block containing this inode
  get_block(mip->dev, block, buf);
  ip = (INODE *)buf + offset;
  *ip = mip->INODE;
  put_block(mip->dev, block, buf);
  midalloc(mip);
}

int search(MINODE *mip, char *name)
{
  // search for name in (DIRECT) data blocks of mip->INODE
  // return its ino
  // Code in Chapter 11.7.2

  int i;
  char *cp, temp[256], sbuf[BLKSIZE];
  DIR *dp;
  for (i = 0; i < 12; i++)
  {
    if (mip->INODE.i_block[i] == 0)
      return 0;
    get_block(mip->dev, mip->INODE.i_block[i], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    while (cp < sbuf + BLKSIZE)
    {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      //printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
      if (strcmp(name, temp) == 0)
      {
        //printf("found %s : inumber = %d\n", name, dp->inode);
        return dp->inode;
      }

      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
}

int getino(char *pathname)
{
  // return ino of pathname
  // Code in Chapter 11.7.2
  MINODE *mip;
  int i, ino;
  if (strcmp(pathname, "/") == 0)
  {
    return 2;
  }

  if (pathname[0] == '/')
  {
    mip = root;
    dev = root->dev;
  }

  else
  {
    mip = running->cwd;
    dev = running->cwd->dev;
  }

  mip->refCount++;

  tokenize(pathname);

  for (i = 0; i < nname; i++)
  {
    if (!S_ISDIR(mip->INODE.i_mode))
    {
      printf("%s is not a directory\n", name[i]);
      iput(mip);
      return 0;
    }

    if (strcmp(pathname, "..") == 0 && mip->ino == 2)
    {
      for (int i = 0; i < NMTABLE; ++i)
      {
        if (mip->dev != mtable[i].dev && mtable[i].dev != 0)
        {
          dev = mtable[i].dev;
          iput(mip);
          mip = mtable[i].mntDirPtr;
          break;
        }
      }
    }

    else if (mip->mounted == 1)
    {
      printf("Mounted\n");
      dev = mip->mountptr->dev;
      iput(mip);
      mip = iget(dev, 2);
    }

    ino = search(mip, name[i]);
    if (!ino)
    {
      printf("No such component name %s\n", name[i]);
      iput(mip);
      return 0;
    }

    iput(mip);
    mip = iget(dev, ino);
  }
  if (mip->mounted == 1)
  {
    dev = mip->mountptr->dev;
    ino = 2;
  }

  iput(mip);
  return ino;
}

int findmyname(MINODE *parent, u32 myino, char *myname)
{
  // WRITE YOUR code here:
  // search parent's data block for myino;
  // copy its name STRING to myname[ ];
  char blockBuf[BLKSIZE], temp[256];
  char *cp;
  DIR *dp;
  for (int i = 0; i < 12; i++)
  {
    if (parent->INODE.i_block[i] == 0)
      return 0;
    get_block(parent->dev, parent->INODE.i_block[i], blockBuf);
    dp = (DIR *)blockBuf;
    cp = blockBuf;
    while (cp < blockBuf + BLKSIZE)
    {
      // strncpy(temp, dp->name, dp->name_len);
      // temp[dp->name_len] = 0;
      // printf("LENGTH: %d\n", strlen(dp->name));
      // printf("DP_NAME: %d\n", dp->name_len)
      strcpy(temp, dp->name);
      if (myino == dp->inode)
      {
        // printf("found INO, it's name is %s\n", temp);
        strcpy(myname, temp);
        return;
      }

      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
}

int findino(MINODE *mip, u32 *myino) // myino = ino of . return ino of ..
{
  // mip->a DIR minode. Write YOUR code to get mino=ino of .
  //                                         return ino of ..
  *myino = mip->ino;
  int parentIno = search(mip, "..");
  // printf("Parent ino is:%d\n", parentIno);
  return parentIno;
}

// LEVEL 1
int tst_bit(char *buf, int bit)
{
  return buf[bit / 8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit)
{
  buf[bit / 8] |= (1 << (bit % 8));
}
int decFreeInodes(int dev)
{
  char buf[BLKSIZE];
  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int decFreeBnodes(int dev)
{
  char buf[BLKSIZE];
  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int incFreeInodes(int dev)
{
  char buf[BLKSIZE];
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

int incFreeBnodes(int dev)
{
  char buf[BLKSIZE];
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
}

int ialloc(int dev) // allocate an inode number from inode_bitmap
{
  int i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i = 0; i < ninodes; i++)
  {
    if (tst_bit(buf, i) == 0)
    {
      set_bit(buf, i);
      put_block(dev, imap, buf);
      // update free inode count in SUPER and GD
      decFreeInodes(dev);
      printf("allocated ino = %d\n", i + 1); // bits count from 0; ino from 1
      return i + 1;
    }
  }
  return 0;
}

int balloc(int dev)
{
  int i;
  char buf[BLKSIZE];

  // read inode_bitmap block

  get_block(dev, bmap, buf);

  for (i = 0; i < nblocks; i++)
  {
    if (tst_bit(buf, i) == 0)
    {
      set_bit(buf, i);
      put_block(dev, bmap, buf);
      decFreeBnodes(dev);
      printf("allocated bno = %d\n", i + 1); // bits count from 0; ino from 1
      return i + 1;
    }
  }
  return 0;
}

int clr_bit(char *buf, int bit)
{
  buf[bit / 8] &= ~(1 << (bit % 8));
}

int idalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];

  if (ino > ninodes)
  {
    printf("inumber %d out of range\n", ino);
    return;
  }
  get_block(dev, imap, buf);
  clr_bit(buf, ino - 1);
  put_block(dev, imap, buf);
  incFreeInodes(dev);
}

int bdalloc(int dev, int bno)
{
  int i;
  char buf[BLKSIZE];
  if (bno > nblocks)
  {
    printf("bnumber %d out of range\n", bno);
    return;
  }
  get_block(dev, bmap, buf);
  clr_bit(buf, bno - 1);
  put_block(dev, bmap, buf);
  incFreeBnodes(dev);
}