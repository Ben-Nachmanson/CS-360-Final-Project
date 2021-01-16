/****************************************************************************
*                   KCW testing ext2 file system                            *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>

#include "type.h"

// globals
MINODE minode[NMINODE];
MINODE *root;

MTABLE mtable[NMTABLE];

PROC proc[NPROC],
    *running;

OFT openFiles[NOFT];

char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int n;           // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start, iblock;

MINODE *iget();

#include "util.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"
#include "open_close_lseek.c"
#include "read_cat.c"
#include "write_cp.c"
#include "mount_umount.c"
int init()
{
  int i, j;
  MINODE *mip;
  PROC *p;

  printf("init()\n");

  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mountptr = 0;
  }
  for (i = 0; i < NPROC; i++)
  {
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
    p->status = FREE;
    for (j = 0; j < NFD; j++)
      p->fd[j] = 0;
  }

  for (i = 0; i < NOFT; ++i)
  {
    openFiles[i].refCount = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root(char *diskImage)
{
  MTABLE *mp = &mtable[0];
  int ino;
  char buf[BLKSIZE];

  printf("checking EXT2 FS ....");
  if ((fd = open(diskImage, O_RDWR)) < 0)
  {
    printf("open %s failed\n", diskImage);
    exit(1);
  }
  dev = fd;

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53)
  {
    printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
    exit(1);
  }
  printf("EXT2 FS OK\n");
  ninodes = mp->ninodes = sp->s_inodes_count;
  nblocks = mp->nblocks = sp->s_blocks_count;
  mp->dev = dev;

  strcpy(mp->devName, diskImage);
  strcpy(mp->mntName, "/");

  get_block(dev, 2, buf);
  gp = (GD *)buf;

  bmap = mp->bmap = gp->bg_block_bitmap;
  imap = mp->imap = gp->bg_inode_bitmap;
  inode_start = mp->iblock = gp->bg_inode_table;
  iblock = mp->iblock = gp->bg_inode_table;

  root = iget(dev, 2);
  mp->mntDirPtr = root;
  root->mountptr = mp;

  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);
}

char *disk = "diskimage";
int main(int argc, char *argv[])
{
  char line[128], cmd[32], pathname[128];
  init();

  if (argc > 1)
    disk = argv[1];

  mount_root(disk);
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  while (1)
  {
    char linkFile[128];
    printf("input command : [ls|cd|pwd|mkdir|rmdir|creat|link|unlink|open|close|read|cat|write|cp|pfd|lseek|mount|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line) - 1] = 0;

    if (line[0] == 0)
      continue;
    pathname[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, linkFile);
    printf("cmd=%s pathname=%s\n", cmd, pathname);

    if (strcmp(cmd, "ls") == 0)
      ls(pathname);
    if (strcmp(cmd, "cd") == 0)
      chdir(pathname);
    if (strcmp(cmd, "pwd") == 0)
      pwd(running->cwd);
    if (strcmp(cmd, "mkdir") == 0)
      make_dir(pathname);
    if (strcmp(cmd, "creat") == 0)
      creat_file(pathname);
    if (strcmp(cmd, "rmdir") == 0)
      rmdir(pathname);
    if (strcmp(cmd, "link") == 0)
      link(pathname, linkFile);
    if (strcmp(cmd, "unlink") == 0)
      unlink(pathname);
    if (strcmp(cmd, "symlink") == 0)
      symlink(linkFile, pathname);
    if (strcmp(cmd, "open") == 0)
      open_userInput(pathname);
    if (strcmp(cmd, "close") == 0)
      close_userInput();
    if (strcmp(cmd, "read") == 0)
      read_file();
    if (strcmp(cmd, "cat") == 0)
      cat(pathname);
    if (strcmp(cmd, "write") == 0)
      write_file();
    if (strcmp(cmd, "cp") == 0)
      cp(pathname, linkFile);
    if (strcmp(cmd, "mv") == 0)
      mv(pathname, linkFile);
    if (strcmp(cmd, "pfd") == 0)
      pfd();
    if (strcmp(cmd, "mount") == 0)
      mount(pathname, linkFile);
    if (strcmp(cmd, "umount") == 0)
      umount(pathname);
    if (strcmp(cmd, "lseek") == 0)
      my_lseek();
    if (strcmp(cmd, "quit") == 0)
      quit();

    if (running->cwd->dev != dev)
      dev = running->cwd->dev;
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
