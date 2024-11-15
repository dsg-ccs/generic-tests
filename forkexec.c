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
int main(int argc, char **argv, char **envp) {
  pid_t cpid,w;
  int status;
  int localvar;
  int *heapptr;
  static int staticvar;
  printf("Main program %s started with %d args\n",argv[0],argc);
  cpid = fork();
  if (cpid == -1) {
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
  }   
  return 0;
}
