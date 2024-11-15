#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include<sys/ipc.h>
#include<sys/shm.h>
#include<string.h>
#include<errno.h>

#define BUF_SIZE 1024
#define SHM_KEY1 0x1234
#define SHM_KEY2 0x5678
#define SLEEPTIME 1
#define DOITERS 2

struct shmseg {
   int cnt;
   int complete;
   char buf[BUF_SIZE];
};
int fill_buffer(char * bufptr, int size);

int fill_buffer(char * bufptr, int size) {
   static char ch = 'A';
   int filled_count;
   
   //printf("size is %d\n", size);
   memset(bufptr, ch, size - 1);
   bufptr[size-1] = '\0';
   if (ch > 122)
   ch = 65;
   if ( (ch >= 65) && (ch <= 122) ) {
      if ( (ch >= 91) && (ch <= 96) ) {
         ch = 65;
      }
   }
   filled_count = strlen(bufptr);
   
   //printf("buffer count is: %d\n", filled_count);
   //printf("buffer filled is:%s\n", bufptr);
   ch++;
   return filled_count;
}

/*
 * This program forks and then communicates via system v shared memory
 */


int main(int argc, char**argv, char**envp)
{
    int child;
    FILE* file = stdout;
    setbuf(stdout,NULL);

    if ((child = fork()) == -1)
        perror("fork");
    else if (child) {   /* This is the parent. */
      int shmid, shmid2, numtimes;
      struct shmseg *shmp;
      char *bufptr;
      int spaceavailable;
      shmid = shmget(SHM_KEY1, sizeof(struct shmseg), 0644|IPC_CREAT);
      if (shmid == -1) {
	perror("Shared memory get of key1 failed");
	return 1;
      }
      shmid2 = shmget(SHM_KEY2, sizeof(struct shmseg), 0644|IPC_CREAT);
      if (shmid2 == -1) {
	perror("Shared memory get of key2 failed");
	return 2;
      }
      shmid2 = shmget(SHM_KEY2, sizeof(struct shmseg), 0644|IPC_CREAT|IPC_EXCL);
      if (shmid2 != -1) {
	perror("Shared memory get of key2 a second time succeeded");
	return 2;
      }
   
      // Attach to the segment to get a pointer to it.
      shmp = shmat(shmid, NULL, 0);
      if (shmp == (void *) -1) {
	perror("Shared memory attach");
	return 1;
      }
      printf("Writing Process: shmid = 0x%x shmaddr = 0x%p\n", shmid,shmp);
      fflush(file);
   
      /* Transfer blocks of data from buffer to shared memory */
      bufptr = shmp->buf;
      spaceavailable = BUF_SIZE;
      for (numtimes = 0; numtimes < DOITERS; numtimes++) {
	shmp->cnt = fill_buffer(bufptr, spaceavailable);
	shmp->complete = 0;
	printf("Writing Process: Shared Memory Write: Wrote %d bytes\n", shmp->cnt);
	fflush(file);
	bufptr = shmp->buf;
	spaceavailable = BUF_SIZE;
	sleep(SLEEPTIME);
      }
      printf("Writing Process: Wrote %d times\n", numtimes);
      fflush(file);
      shmp->complete = 1;

      if (shmdt(shmp) == -1) {
	perror("shmdt");
	return 1;
      }

      if (shmctl(shmid, IPC_RMID, 0) == -1) {
	perror("shmctl");
	return 1;
      }
      printf("Writing Process: Complete\n");
      fflush(file);
      return 0;
    } else {        /* This is the child. */
      int shmid;
      struct shmseg *shmp;
      shmid = shmget(SHM_KEY1, sizeof(struct shmseg), 0644|IPC_CREAT);
      if (shmid == -1) {
	perror("Shared memory");
	return 1;
      }
   
      // Attach to the segment to get a pointer to it.
      shmp = shmat(shmid, NULL, 0);
      if (shmp == (void *) -1) {
	perror("Shared memory attach");
	return 1;
      }
   
      printf("Reading Process: shmid = 0x%x shmaddr = 0x%p\n", shmid,shmp);
      fflush(file);
      /* Transfer blocks of data from shared memory to stdout*/
      while (shmp->complete != 1) {
	printf("segment contains : \n\"%s\"\n",shmp->buf);
	fflush(file);
	if (shmp->cnt == -1) {
	  perror("read");
	  return 1;
	}
	printf("Reading Process: Shared Memory: Read %d bytes\n", shmp->cnt);
	fflush(file);
	sleep(SLEEPTIME);
      }
      printf("Reading Process: Reading Done, Detaching Shared Memory\n");
      fflush(file);
      if (shmdt(shmp) == -1) {
	perror("shmdt");
	return 1;
      }
      printf("Reading Process: Complete\n");
      fflush(file);
      return 0;
    }
}
