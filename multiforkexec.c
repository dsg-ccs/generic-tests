#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>

FILE* file;

int globalvar;
/*
  compile as
  gcc -o forktest fork.c 
 */
int main(int argc, char **argv, char **envp) {
  pid_t cpid,w;
  int status;
  int localvar;
  int *heapptr;
  int forkno, numforks;
  FILE* file = stdout;
  
  static int staticvar;
  if (argc > 1) {
    numforks = atoi(argv[1]);
    argc--;
    argv++;
  } else {
    numforks = 5;
  }
  printf("%s will fork %d times\n",argv[0],numforks);
      fflush(file);
      
  heapptr = (int *)malloc(8*sizeof(int));
  localvar=0x01;
  staticvar = 0x11;
  globalvar = 0x21;
  heapptr[0]= 0x31;
  printf("Initial local, static, global, heap = %02x %02x %02x %02x\n",localvar,staticvar,globalvar,heapptr[0]);
      fflush(file);

  for (forkno=0; forkno<numforks; forkno++) {
    cpid = fork();
    if (cpid == -1) {
      printf("Fork %d failed\n",forkno);
      perror("fork failed");
      exit(-1);
    }

    if (cpid == 0) {
      if (argc == 1) {
	printf("Need name of command to exec as first argument\n");
      } else {
	if (execve(argv[1], argv+1, envp) == -1) {
	  printf("Failed to start %s with %d args\n",argv[1], argc);
	  perror("Could not execve");
	}
      }
    } else {
      /* Parent */
      printf("parent before check if child exitted local, static, global = %02x %02x %02x %02x\n",localvar,staticvar,globalvar,heapptr[0]);
      fflush(file);
      do {
	w = waitpid(cpid,&status, WUNTRACED | WCONTINUED);
	if (w == -1) {
	  perror("Waitpid failed");
	  exit(-1);
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
      printf("parent after child exitted local, static, global = %02x %02x %02x %02x\n",localvar,staticvar,globalvar,heapptr[0]);
      fflush(file);
      localvar  = 0x03;
      staticvar = 0x13;
      globalvar = 0x23;
      heapptr[0]= 0x33;
      printf("parent changed local, static, global = %02x %02x %02x %02x\n",localvar,staticvar,globalvar,heapptr[0]);
      fflush(file);
    }   
  }
  return 0;
}
