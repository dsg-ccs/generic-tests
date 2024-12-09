#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#define _GNU_SOURCE


/*
  compile as
  gcc -o self.o self.c 
 */
void spawnforks(int numforks,int numlevels);
void waitchildren(int levno);
FILE* file;

int main(int argc, char **argv) {
  int numforks, numlevels;
  file = stdout;
  if (argc > 1) {
    numforks = atoi(argv[1]);
  } else {
    numforks = 5;
  }
  if (argc > 2) {
    numlevels = atoi(argv[2]);
  } else {
    numlevels = 1;
  }
  printf("%s will fork %d times on %d levels\n",argv[0],numforks,numlevels);
  fflush(file);
  spawnforks(numforks,numlevels);
  printf("%s  forked %d times on %d levels\n",argv[0],numforks,numlevels);
  fflush(file);
}
  
// For some reason this is not declared in header files
int gettid(void);

// Function that will be run by each thread
void* print_thread_id(void* id) {
    int thread_id = *((int*)id);
    int *exitval = malloc(sizeof(int));
    FILE* file = stdout;
    printf(" Thread %d is running as pid, tid: %d,%d\n", thread_id,getpid(),gettid());
    fflush(file);
    sleep(2);
    printf(" Thread %d is running as pid, tid: %d,%d ready to exit\n", thread_id,getpid(),gettid());
    fflush(file);
    *exitval=thread_id;
    pthread_exit((void *)exitval);
}

#define MAXTHREADS 20
void spawnthreads(int num_threads) {
  int i;
  pthread_t threads[MAXTHREADS];
  int thread_ids[MAXTHREADS];
  if (num_threads > MAXTHREADS) {
    num_threads = MAXTHREADS;
  }

  // Create the threads
  for(int i = 0; i < num_threads; i++) {
    thread_ids[i] = i + 1;
    int rc = pthread_create(&threads[i], NULL, print_thread_id, (void*)&thread_ids[i]);
    if (rc) {
      printf("Error: Unable to create thread %d\n", i + 1);
      exit(-1);
    }
  }

  // Wait for all threads to finish
  for(int i = 0; i < num_threads; i++) {
    int *threadstatus;
    pthread_join(threads[i], (void**)&threadstatus);
    printf("Joined with thread %ld which exited 0x%x\n", threads[i],*threadstatus);
    fflush(file);
    free(threadstatus);
  }
}

void spawnforks(int numforks, int numlevels) {
  int forkno;
  pid_t cpid,w;
  if (numlevels == 0) {
    spawnthreads(numforks);
  } else {
    for (forkno=0; forkno<numforks; forkno++) {
      cpid = fork();
      if (cpid == -1) {
	printf("Fork %d failed\n",forkno);
	perror("fork failed");
	exit(-1);
      }

      if (cpid == 0) {
	/* Child */
	// Do some stuff
	printf("Level %d, Child %d: PID %d, parent PID %d, uid %d, euid %d, gid %d, egid %d\n",
	       numlevels, forkno, getpid(), getppid(), getuid(), geteuid(), getgid(), getegid());
	fflush(file);
	// Give others a chance to start before exitting
	sleep(2);
	spawnforks(numforks, numlevels-1);
	exit(forkno);
      } else {
	/* Parent */
	// Just keep creating more children
      }
    }
    waitchildren(numlevels);
  }
}

void waitchildren(int levno) {
  pid_t cpid,w;
  int status;
  // Now wait on children ending - probably will not see each exit
  do {
    w = waitpid(0,&status, WUNTRACED | WCONTINUED);
    if (w == -1) {
      // No more children were left
      break;
    }
    if (WIFEXITED(status)) {
      printf("exited, status=%d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("killed by signal %d\n", WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
      printf("stopped by signal %d\n", WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
      printf("continued\n");
    }
  } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  printf("parent %d after all children exitted\n",levno);
  fflush(file);
  return;
}
