#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
// For some reason this is not declared in header files
int gettid(void);

void* print_thread_id(void* id);


pid_t execorproceed() {
    pid_t childpid,w;
    int ret;
    FILE* file = stdout;
    int status;
    int num_threads = 5;
    pthread_t threads[num_threads];
    int thread_ids[num_threads];
    char pad[]="           ";


    childpid = fork();
    if (childpid == -1) {
      perror("fork failed");
      exit(-1);
    }

    if (childpid == 0) {
      /* Child */
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
	if (i < strlen(pad)) {
	  pad[i]=0;
	}
	pthread_join(threads[i], (void**)&threadstatus);
	printf("Joined with thread %d which exited 0x%x %s\n", i,*threadstatus,pad);
	free(threadstatus);
	if (i < strlen(pad)) {
	  pad[i]=' ';
	}
      }
    } else {
      /* Parent */
      // Do an exec
      char* argv[] = { NULL };
      char* envp[] = { NULL };
      printf("Bed time for forked child\n");
      fflush(file);
      sleep(4);
      printf("Forked child awoke\n");
      fflush(file);
      if (execve("./hello-stat", argv, envp) == -1) {
	printf("Failed to start ./hello-stat\n");
	perror("Could not execve");
      }
    }
}


// Function that will be run by each thread
void* print_thread_id(void* id) {
  char pad[]="           ";
    int thread_id = *((int*)id);
    int *exitval = malloc(sizeof(int));
    FILE* file = stdout;
    if (thread_id < strlen(pad)) {
      pad[thread_id]=0;
    }
    printf("Our thread %d is running as pid, tid: %d,%d %s\n", thread_id,getpid(),gettid(),pad);
    fflush(file);
    *exitval=thread_id;
    pthread_exit((void *)exitval);
}

int main(int argc, char**argv, char**envp) {

    // Split into two threads, one execs - other does threading
    execorproceed();


    return 0;
}
