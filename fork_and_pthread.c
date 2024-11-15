#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
// For some reason this is not declared in header files
int gettid(void);

pid_t callsys(const char*name) {
    pid_t cpid,w;
    int ret;
    FILE* file = stdout;
    cpid = fork();
    if (cpid == -1) {
      perror("fork failed");
      exit(-1);
    }

    if (cpid == 0) {
      /* Child */
      // Run server thread
      printf("Bed time for forked child\n");
      fflush(file);
      sleep(4);
      printf("Forked child awoke\n");
      fflush(file);
      exit(17);
    } else {
      /* Parent */
      // Give the server some time to start up
      return cpid;
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
    int num_threads = 5;
    pthread_t threads[num_threads];
    int thread_ids[num_threads];
    char pad[]="           ";
    pid_t childpid,w;
    int status;
    FILE* file = stdout;

    // Try a call to system
    if (argc <= 1) {
      childpid = callsys("./test.sh");
    } else {
      childpid = callsys(argv[1]);
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

    // Wait for childpid to finish
    w = waitpid(childpid,&status, WUNTRACED | WCONTINUED);
    if (w == -1) {
      perror("Waitpid failed");
      exit(-1);
    }
    if (WIFEXITED(status)) {
      printf("childpid exited, status=%d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("childpid killed by signal %d\n", WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
      printf("childpid stopped by signal %d\n", WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
      printf("childpid continued\n");
    }
    printf("Childpid fork done\n");
    fflush(file);

    return 0;
}
