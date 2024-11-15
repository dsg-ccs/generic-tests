#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>


int globalvar;
/*
  compile as
  gcc -o forktest fork.c 
 */
int main(int argc, char **argv) {
  pid_t cpid,w;
  int status;
  int localvar;
  int *heapptr;
  static int staticvar;
  printf("Main program %s started with %d args\n",argv[0],argc);
  heapptr = (int *)malloc(8*sizeof(int));
  localvar=0x01;
  staticvar = 0x11;
  globalvar = 0x21;
  heapptr[0]= 0x31;
  printf("Initial local, static, global, heap = %02x %02x %02x %02x\n",localvar,staticvar,globalvar,heapptr[0]);
  
  cpid = fork();
  if (cpid == -1) {
    perror("fork failed");
    exit(-1);
  }

  if (cpid == 0) {
    /* Child */
    printf("Child PID %d, parent PID %d, uid %d, euid %d, gid %d, egid %d\n",
	   getpid(), getppid(), getuid(), geteuid(), getgid(), getegid());
    printf("child local, static, global = %02x %02x %02x %02x\n",localvar,staticvar,globalvar,heapptr[0]);
    localvar  = 0x02;
    staticvar = 0x12;
    globalvar = 0x22;
    heapptr[0]= 0x32;
    printf("child changed local, static, global = %02x %02x %02x %02x\n",localvar,staticvar,globalvar,heapptr[0]);
  } else {
    /* Parent */
    printf("parent before check if child exitted local, static, global = %02x %02x %02x %02x\n",localvar,staticvar,globalvar,heapptr[0]);
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
    localvar  = 0x03;
    staticvar = 0x13;
    globalvar = 0x23;
    heapptr[0]= 0x33;
    printf("parent changed local, static, global = %02x %02x %02x %02x\n",localvar,staticvar,globalvar,heapptr[0]);
  }   
  return 0;
}
