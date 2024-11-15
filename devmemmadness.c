#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#define MMIO_START 0x60000000
#define MMIO_SIZE 0x1000
#define TESTVAL 0x12345678
int main(int argc, char**argv, char**envp)
{
  int fd;
  unsigned long int *mmio_region;
  unsigned long int val1,val2;

  fd = open("/dev/mem",O_RDWR);
  if (fd == -1) {
    fprintf(stderr,"open of /dev/mem failed\n");
    perror("open");
  } else {
    printf("open of /dev/mem at fd %d\n",fd);
  }

  mmio_region = (unsigned long int *) mmap(0,MMIO_SIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,MMIO_START);
  if (mmio_region == MAP_FAILED) {
    fprintf(stderr,"mmap of /dev/mem failed\n");
  } else {
    printf("mmap of /dev/mem at 0x%p\n",mmio_region);

    val1 = mmio_region[0];
    mmio_region[0]=TESTVAL;
    val2 = mmio_region[0];
    printf("Read as %lx, set to %lx, then read %lx\n",val1,(unsigned long int) TESTVAL,val2);
  }
  
  return 0;
}
