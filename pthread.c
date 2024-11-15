#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>

// For some reason this is not declared in header files
int gettid(void);

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
    int num_threads = 5;
    pthread_t threads[num_threads];
    int thread_ids[num_threads];
    char pad[]="           ";
    FILE* file = stdout;

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
      fflush(file);
      free(threadstatus);
      if (i < strlen(pad)) {
	pad[i]=' ';
      }
    }

    return 0;
}
