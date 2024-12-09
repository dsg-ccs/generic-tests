#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#define TESTVAL 0x12345678
int main(int argc, char**argv, char**envp)
{
  int fd, nbytes;
  char buf[0x100];
  const char val[]="Test string";
  unsigned long int val1,val2;
  unsigned long int *region;
  setbuf(stderr,NULL);
  setbuf(stdout,NULL);
  
  region = (unsigned long int *) mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,0,0);
  printf("Got a mmap region at %p\n",region);
  region[0] = (unsigned long int)TESTVAL;

  fd = open("/qemu/synfiles/fromguest",O_WRONLY|O_CREAT|O_TRUNC,S_IWUSR);
  if (fd == -1) {
    fprintf(stderr,"open of fromguest failed\n");
    perror("open");
  } else {
    printf("open of fromguest at fd %d\n",fd);
  }
  close(fd);

  fd = open("/FAKEME/fakefile",O_RDWR,S_IRUSR | S_IWUSR);
  if (fd == -1) {
    fprintf(stderr,"open of FAKEME failed\n");
    perror("open");
  } else {
    printf("open of /FAKEME/fakefile at fd %d\n",fd);
  }

  nbytes = read(fd,buf,0x100);
  if (nbytes != 0x100)
    printf("read of fakefile failed only got %d bytes\n",nbytes);
  
  lseek(fd,0x80,SEEK_SET);
  nbytes = write(fd,val,sizeof(val));
  if (nbytes != sizeof(val))
    printf("write of fakefile failed only wrote %d bytes\n",nbytes);

  close(fd);
  
  return 0;
}
