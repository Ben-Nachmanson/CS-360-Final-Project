/************* cd_ls_pwd.c file **************/

int chdir(char *pathname)
{
  printf("chdir %s\n", pathname);
  printf("PATHNAME:%s\n", pathname);
  //printf("under construction READ textbook HOW TO chdir!!!!\n");
  // READ Chapter 11.7.3 HOW TO chdir
  if (strcmp(pathname, "") == 0)
  {
    iput(running->cwd);
    running->cwd = root;
    return;
  }

  int ino = getino(pathname);

  if (ino == 0)
  {
    printf("getino failed\n");
    return;
  }
  MINODE *mip = iget(dev, ino);

  if (S_ISDIR(mip->INODE.i_mode) == 0)
  {
    printf("cd failed; not a directory\n");
    iput(mip);
    return;
  }

  iput(running->cwd);
  running->cwd = mip;
}

int ls_file(MINODE *mip, char *name)
{
  //printf("ls_file: to be done: READ textbook for HOW TO!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  int isLink = 0;
  time_t atime = mip->INODE.i_atime;
  char *fileTime = ctime(&atime);
  fileTime[strlen(fileTime) - 1] = '\0';

  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  //copied from our Lab 5 code
  if ((mip->INODE.i_mode & 0xF000) == 0x8000) // if (S_ISREG())
    printf("%c", '-');
  if ((mip->INODE.i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
    printf("%c", 'd');
  if ((mip->INODE.i_mode & 0xF000) == 0xA000) // if (S_ISLNK())
  {
    printf("%c", 'l');
    isLink = 1;
  }

  for (int i = 8; i >= 0; i--)
  {
    if (mip->INODE.i_mode & (1 << i)) // print r|w|x
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]);
    // or print -
  }
  printf(" ");
  printf("%d  %d  %d  %s  %d  %s", mip->INODE.i_links_count, mip->INODE.i_uid, mip->INODE.i_gid, fileTime, mip->INODE.i_size, name);

  if (isLink == 1)
  {
    char link[128];
    readlink(name, link);
    printf(" -> %s\n", link);
  }

  else
  {
    printf("\n");
  }
}

int ls_dir(MINODE *mip)
{
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  // Assume DIR has only one data block i_block[0]
  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE)
  {
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    //printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
    MINODE *tempMINODE = iget(dev, dp->inode);
    ls_file(tempMINODE, temp);
    iput(tempMINODE);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  printf("\n");
}

int ls(char *pathname)
{
  printf("ls %s\n", pathname);
  if (strcmp(pathname, "") == 0)
  {
    ls_dir(running->cwd);
  }

  else
  {
    int ino = getino(pathname);

    if (ino == 0)
    {
      printf("ls failed: %s does not exist\n", pathname);
      return;
    }

    MINODE *mip = iget(dev, ino);

    if (S_ISREG(mip->INODE.i_mode) > 0)
    {
      ls_file(mip, pathname);
    }

    else
    {
      ls_dir(mip);
    }

    iput(mip);
  }
}

int rpwd(MINODE *wd)
{
  if (wd == root)
  {
    return;
  }

  else if (wd->ino == 2)
  {
    return;
  }

  int myIno, parentIno;
  parentIno = findino(wd, &myIno);

  MINODE *pip = iget(dev, parentIno);

  char name[128];
  findmyname(pip, myIno, &name);
  rpwd(pip);
  printf("/%s", name);
}

char *pwd(MINODE *wd)
{
  if (wd == root)
  {
    printf("/\n");
  }

  else
  {
    if (wd->dev != root->dev)
    {
      for (int i = 0; i < NMTABLE; ++i)
      {
        if (mtable[i].dev == wd->dev)
        {
          printf("%s", mtable[i].mntName);
          break;
        }
      }
    }

    rpwd(wd);
    printf("\n");
  }
}
