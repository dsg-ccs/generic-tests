#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAXTHREADS 20
#define PIPE_NAME "/tmp/heterothreads_pipe"
#define BUFFER_SIZE


// For some reason this is not declared in header files
int gettid(void);

struct threaddef {
  pthread_t thread;
  int id;
  void *(*func)(void* );
};

// Function that simply prints info and exits
void* print_thread_id(void* id) {
    int thread_id = *((int*)id);
    int *exitval = malloc(sizeof(int));
    printf("Our thread %d is running as pid, tid: %d,%d\n", thread_id,getpid(),gettid());
    sleep(1);
    printf("Our thread %d is running as pid, tid: %d,%d ready to exit\n", thread_id,getpid(),gettid());
    *exitval=thread_id;
    pthread_exit((void *)exitval);
}

// Function that waits for an character to be input on stdin
void* waitkeyboard(void* id){
    char ch;
    int *exitval = malloc(sizeof(int));
    int thread_id = *((int*)id);
    printf("Thread %d will wait for char to be entered\n",thread_id);
    ch = getchar();
    if (ch == EOF) {
      printf("Interrupted\n");
    } else {
      printf("Got %c\n",ch);
    }
    *exitval=ch;
    pthread_exit((void *)exitval);
}

// Function reads from a fake device name until conditions are met
void* waitfakedev(void* id){
    char input[11];
    int *exitval = malloc(sizeof(int));
    int thread_id = *((int*)id);
    int fd;
    int nb;
    input[10]=0;
    fd = open("/dev/fakeme",O_RDONLY);
    if (fd == -1) {
      printf("No /dev/fakeme\n");
      *exitval=-1;
      pthread_exit((void *)exitval);
    }
    printf("Thread %d will wait for string 'bingo' to be read from fake dev\n",thread_id);
    while (true) {
      nb = read(fd,input,10);
      if (nb != 10) {
	printf("Not enough input try again %d\n",nb);
	if (nb <= 0) sleep(1);
	continue;
      }
      if (strncmp("bingo",input,5) == 0) {
	printf("got bingo, return %c\n",input[9]);
	break;
      } else {
	printf("bad input %s not bingo\n",input);
	sleep(1);
      }
    }
    *exitval=input[9];
    pthread_exit((void *)exitval);
}

// Function reads from a fake file until conditions are met
void* waitfakefile(void* id){
    char input[11];
    int *exitval = malloc(sizeof(int));
    int thread_id = *((int*)id);
    int fd;
    int nb;
    input[10]=0;
    fd = open("/FAKEME/fakename",O_RDONLY);
    if (fd == -1) {
      printf("No file FAKEME\n");
      *exitval=-1;
      pthread_exit((void *)exitval);
    }
    printf("Thread %d will wait for string 'bingo' to be read from fake dev\n",thread_id);
    while (true) {
      nb = read(fd,input,10);
      if (nb != 10) {
	printf("Not enough input (%d) try again\n",nb);
	if (nb == 0) sleep(1);
	continue;
      }
      if (strncmp("bingo",input,5) == 0) {
	printf("got bingo, return %c\n",input[9]);
	break;
      } else {
	printf("bad input %s not bingo\n",input);
      }
    }
    *exitval=input[9];
    pthread_exit((void *)exitval);
}

// Function reads from a fake file until conditions are met
void* pipewriter(void* id){
    char sendbuf[100];
    int *exitval = malloc(sizeof(int));
    int thread_id = *((int*)id);
    int fd = open(PIPE_NAME, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open pipe for writing");
        pthread_exit(NULL);
    }

    const char *messages[] = {
        "1st message",
        "2nd message",
        "last message"
    };

    for (int i = 0; i < 3; i++) {
      sprintf(sendbuf, "From %d: %s", thread_id,messages[i]);
      write(fd, sendbuf, strlen(sendbuf) + 1);
      printf("Writer: Sent message: %s\n", sendbuf);
    }

    close(fd);
    *exitval='X';
    pthread_exit((void *)exitval);
}
void* pipereader(void* id){
    char readbuf[100];
    int *exitval = malloc(sizeof(int));
    int thread_id = *((int*)id);
    int fd = open(PIPE_NAME, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open pipe for writing");
        pthread_exit(NULL);
    }

    for (int i = 0; i < 3; i++) {
      ssize_t bytes_read = read(fd, readbuf, 100);
      if (bytes_read > 0) {
	printf("Reader %d: Received message: %s\n", thread_id, readbuf);
      }
    }

    close(fd);
    *exitval=readbuf[10];
    pthread_exit((void *)exitval);
}

int main(int argc, char**argv, char**envp) {
    struct threaddef defs[MAXTHREADS];
    setbuf(stdout,NULL);
    int numthreads = 0;
    defs[numthreads].func = print_thread_id;
    numthreads++;
    defs[numthreads].func = waitkeyboard;
    numthreads++;
    defs[numthreads].func = waitfakedev;
    numthreads++;
    defs[numthreads].func = waitfakefile;
    numthreads++;
    defs[numthreads].func = pipewriter;
    numthreads++;
    defs[numthreads].func = pipereader;
    numthreads++;
    if (access("/tmp",F_OK) == 0 ) {
      chmod("/tmp",S_IXUSR|S_IWUSR|S_IRUSR);
    } else {
      mkdir("/tmp",S_IXUSR|S_IWUSR|S_IRUSR);
    };
    if (access(PIPE_NAME,F_OK) != 0 ) {
      if (mkfifo(PIPE_NAME, 0666) == -1) {
        perror("Failed to create named pipe");
        return EXIT_FAILURE;
      }
    }

    printf("%s will create %d threads\n",argv[0],numthreads);
 
    // Create the threads
    for(int i = 0; i < numthreads; i++) {
        defs[i].id = i + 1;
        int rc = pthread_create(&(defs[i].thread), NULL, defs[i].func, (void*)&(defs[i].id));
        if (rc) {
            printf("Error: Unable to create thread %d\n", i + 1);
            exit(-1);
        }
    }

    // Wait for all threads to finish
    for(int i = 0; i < numthreads; i++) {
      int *threadstatus;
      pthread_join(defs[i].thread, (void**)&threadstatus);
      printf("Joined with thread %d which exited 0x%x\n", i,*threadstatus);
      free(threadstatus);
    }

    unlink(PIPE_NAME);
    return 0;
}
