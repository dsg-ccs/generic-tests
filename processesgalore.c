#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
// For some reason this is not declared in header files
int gettid(void);

// Function that will be run by each thread
void* do_thread(void* id) {
  pid_t cpid,w;
  int thread_id = *((int*)id);
  int *exitval = malloc(sizeof(int));
  FILE* file = stdout; // Want file handle for stdout so can flush, 
  int forkno, numforks;
  int status;
  fflush(file); // make write happen here and not buffer.
  numforks = thread_id+2;
  printf("In thread %d create %d forked children PID %d, tid %d\n",thread_id,numforks,getpid(),gettid());
  fflush(file); // make write happen here and not buffer.
  for (forkno=0; forkno<numforks; forkno++) {
    cpid = fork();
    if (cpid == -1) {
      printf("Fork %d failed\n",forkno);
      perror("fork failed");
      fflush(file); // make write happen here and not buffer.
      exit(-1);
    }

    if (cpid == 0) {
      /* Child */
      printf("Child  %d of thread %d: PID %d, tid %d, parent PID %d, uid %d, euid %d, gid %d, egid %d\n",
	     forkno, thread_id, getpid(), gettid(), getppid(), getuid(), geteuid(), getgid(), getegid());
      fflush(file); // make write happen here and not buffer.
      sleep(5*(forkno+1));
      printf("Child  %d of thread %d done\n", forkno, thread_id);
      exit(forkno);
    } else {
      /* Parent */
      //printf("Parent of thread %d continuing\n", thread_id);
      //fflush(file); // make write happen here and not buffer.
    }
  }
  printf("Parent of thread %d: PID %d, tid %d made all children\n", thread_id,getpid(), gettid());
  fflush(file); // make write happen here and not buffer.
  do {
	w = waitpid(cpid,&status, WUNTRACED | WCONTINUED);
	if (w == -1) {
	  perror("Waitpid failed");
	  break;
	}
	if (WIFEXITED(status)) {
	  printf("Parent of thread %d: saw child %d exited, status=%d\n",
		 thread_id, w, WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
	  printf("Parent of thread %d: saw child %d killed by signal %d\n",
		 thread_id, w, WTERMSIG(status));
	} else if (WIFSTOPPED(status)) {
	  printf("Parent of thread %d: saw child %d stopped by signal %d\n",
		 thread_id, w, WSTOPSIG(status));
	} else if (WIFCONTINUED(status)) {
	  printf("Parent of thread %d: saw child %d continued\n",
		 thread_id, w);
	}
	fflush(file); // make write happen here and not buffer.
  } while (!WIFEXITED(status) && !WIFSIGNALED(status));

  *exitval=thread_id;
  pthread_exit((void *)exitval);
}

int main(int argc, char**argv, char**envp) {
    int num_threads = 5;
    pthread_t threads[num_threads];
    int thread_ids[num_threads];
    char pad[]="           ";
    FILE* file = stdout; // Want file handle for stdout so can flush, 

    if (argc > 1) {
      num_threads = atoi(argv[1]);
    } else {
      num_threads = 5;
    }

    // Create the threads
    printf("Creating %d threads\n",num_threads);
    fflush(file); // make write happen here and not buffer.
    for(int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        int rc = pthread_create(&threads[i], NULL, do_thread, (void*)&thread_ids[i]);
        if (rc) {
            printf("Error: Unable to create thread %d\n", i);
            exit(-1);
        }
    }
    printf("%d threads made, sleep a bit\n",num_threads);
    fflush(file); // make write happen here and not buffer.

    sleep(5);

    // Wait for all threads to finish
    for(int i = 0; i < num_threads; i++) {
      int *threadstatus;
      if (i < strlen(pad)) {
	pad[i]=0;
      }
      pthread_join(threads[i], (void**)&threadstatus);
      printf("Joined with thread %d which exited 0x%x %s\n", i,*threadstatus,pad);
      //free(threadstatus);
      if (i < strlen(pad)) {
	pad[i]=' ';
      }
    }

    return 0;
}
