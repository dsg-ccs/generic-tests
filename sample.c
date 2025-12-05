// sample.c
#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/wait.h>
FILE* file;

int fn(void *arg)
{
   printf("\nINFO: This code is running under child process.\n");

   int i = 0;
   
   int n = atoi(arg);

   for ( i = 1 ; i <= 3 ; i++ ) {
      printf("%d * %d = %d\n", n, i, (n*i));
      fflush(file);
   }

   printf("\n");

   return 0;
}

void main(int argc, char *argv[])
{
  int i;
  FILE* file = stdout;
  pid_t cpid,w;
  int status;
   printf("Hello, World!\n");
   for (i = 1; i< argc; i++)
     printf("Arg %d: %s",i,argv[i]);
   fflush(file);

   void *pchild_stack = malloc(1024 * 1024);
   if ( pchild_stack == NULL ) {
      printf("ERROR: Unable to allocate memory.\n");
      exit(EXIT_FAILURE);
   }

   int pid = clone(fn, pchild_stack + (1024 * 1024), SIGCHLD, argv[1]);
   if ( pid < 0 ) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
   }


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
   free(pchild_stack);

   printf("INFO: Child process terminated.\n");
}

