#include <stdio.h>
#include <sys/stat.h>

/*
  compile as
  gcc -o stattest stat.c 
 */
int main(int argc, char**argv)
{
  char *filename;
  struct stat info;
  if (argc == 1) {
    filename=argv[0];
  } else {
    filename=argv[1];
  }
  stat(filename,&info);
  printf("%lld %ld %d %d\n",info.st_dev,info.st_ino,info.st_mode,info.st_nlink);
  printf("%d %d %lld\n",info.st_uid,info.st_gid,info.st_rdev);
  printf("%ld %ld %ld\n",info.st_size,info.st_blksize,info.st_blocks);
  printf("%ld %ld %ld\n",info.st_atime,info.st_mtime,info.st_ctime);
  
  printf("%llx %lx %x %x\n",info.st_dev,info.st_ino,info.st_mode,info.st_nlink);
  printf("%x %x %llx\n",info.st_uid,info.st_gid,info.st_rdev);
  printf("%lx %lx %lx\n",info.st_size,info.st_blksize,info.st_blocks);
  printf("%lx %lx %lx\n",info.st_atime,info.st_mtime,info.st_ctime);
  return 0;
}
