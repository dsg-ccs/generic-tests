// sample.c
#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/wait.h>
FILE* file;

int fn(void *arg)
{
  printf("\nINFO: This code is running under child process with arg at %p.\n",arg);
      fflush(file);

   int i = 0;

   // This should seg fault if no arg given
   int n = *(int *)arg;

   printf("INFO: arg is %d.\n",n);   
      fflush(file);

   // This should fault if value is 0
   int v = 10/n;

   if (n == 1) {
     printf("Sleep for a while so can have signal applied\n");
      fflush(file);
     sleep(2);
   } else {
     printf("Working with %d\n",n);
      fflush(file);
   }

   return n;
}

void main(int argc, char *argv[])
{
  int intarg = 0;
  int i;
  FILE* file = stdout;
  pid_t cpid,w;
  int status;
  int pid; 
   printf("Hello, World!\n");
   for (i = 1; i< argc; i++)
     printf("Arg %d: %s",i,argv[i]);
   fflush(file);

   void *pchild_stack = malloc(1024 * 1024);
   if ( pchild_stack == NULL ) {
      printf("ERROR: Unable to allocate memory.\n");
      exit(EXIT_FAILURE);
   }

   // Call with Null argument should segfault
   pid = clone(fn, pchild_stack + (1024 * 1024), SIGCHLD, 0);
   if ( pid < 0 ) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
   }
   w = waitpid(0,&status, WUNTRACED | WCONTINUED);
   if (WIFSIGNALED(status)) {
     if (WTERMSIG(status) == SIGSEGV )
       printf("killed by segfault as expected\n");
     else
      printf("Expect segfault but killed unexpectedly by signal %d\n", WTERMSIG(status));
   } else {
    if (WIFEXITED(status)) {
      printf("Expect segfault but unexpectedly exited normally, status=%d\n", WEXITSTATUS(status));
    } else {
      printf("Expect segfault but unexpectedly continued or stopped\n");
    }
   }
   fflush(file);

   // Call with argument 0 should fault
   pid = clone(fn, pchild_stack + (1024 * 1024), SIGCHLD, &intarg);
   if ( pid < 0 ) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
   }
   w = waitpid(0,&status, WUNTRACED | WCONTINUED);
   if (WIFSIGNALED(status)) {
     if (WTERMSIG(status) == SIGFPE ) {
       printf("killed by floating point error as expected\n");
     } else {
      printf("Expect fpe but killed unexpectedly by signal %d\n", WTERMSIG(status));
     }
   } else {
    if (WIFEXITED(status)) {
      printf("Expect fpe but unexpectedly exited normally, status=%d\n", WEXITSTATUS(status));
    } else {
      printf("Expect fpe but unexpectedly continued or stopped\n");
    }
   }
   fflush(file);

   // Call with argument 1 will sleep 2 so can send signal
   intarg = 1;
   pid = clone(fn, pchild_stack + (1024 * 1024), SIGCHLD, &intarg);
   if ( pid < 0 ) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
   }
   sleep(1);  // let get started
   kill(pid,SIGTERM); // Then kill
   w = waitpid(0,&status, WUNTRACED | WCONTINUED);
   if (WIFSIGNALED(status)) {
     if (WTERMSIG(status) == SIGTERM )
       printf("killed by sigterm as expected\n");
     else
      printf("Expect sigterm but killed unexpectedly by signal %d\n", WTERMSIG(status));
   } else if (WIFEXITED(status)) {
      printf("Exited normally but expected sigterm , status=%d\n", WEXITSTATUS(status));
   } else if (WIFSTOPPED(status)) {
     printf("stopped by signal %d\n", WSTOPSIG(status));
   } else {
     printf("Unexpectedly continued or stopped\n");
   }
   fflush(file);

   // Call with argument 1 will sleep 2 so can send signal
   intarg = 1;
   pid = clone(fn, pchild_stack + (1024 * 1024), SIGCHLD, &intarg);
   if ( pid < 0 ) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
   }
   sleep(1);  // let get started
   kill(pid,SIGSTOP); // Then stop
   w = waitpid(0,&status, WUNTRACED | WCONTINUED);
   if (WIFSIGNALED(status)) {
      printf("Unexpectedly killed  by signal %d\n", WTERMSIG(status));
   } else if (WIFEXITED(status)) {
      printf("Exited normally as expected, status=%d\n", WEXITSTATUS(status));
   } else if (WIFSTOPPED(status)) {
     if (WSTOPSIG(status) == SIGSTOP)
       printf("stopped by sig stop as expected\n");
     else
       printf("stopped by signal %d not %d\n", WSTOPSIG(status),SIGSTOP);
   } else {
     printf("Unexpectedly continued or stopped\n");
   }
   fflush(file);

   // Call with argument 3 just default to a print
   intarg = 3;
   pid = clone(fn, pchild_stack + (1024 * 1024), SIGCHLD, &intarg);
   if ( pid < 0 ) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
   }
   w = waitpid(0,&status, WUNTRACED | WCONTINUED);
   if (WIFSIGNALED(status)) {
      printf("Unexpectdely killed  by signal %d\n", WTERMSIG(status));
   } else {
    if (WIFEXITED(status)) {
      printf("Exited normally as expected, status=%d\n", WEXITSTATUS(status));
    } else {
      printf("Unexpectedly continued or stopped\n");
    }
   }
   fflush(file);



    free(pchild_stack);

   printf("INFO: Child process terminated.\n");
}

