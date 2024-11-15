#include <stdio.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>

void callsys(const char*name) {
    int ret;
    ret = system(name);
    if (ret == -1) {
      perror("Call to system failed");
    } 
    if (WIFEXITED(ret)) {
      if (WEXITSTATUS(ret) == 127){
	printf("Call to system(%s) could not be executed\n", name);
      } else {
	printf("Call to system(%s) returned %d\n", name, WEXITSTATUS(ret));
      }
    } else if (WIFSIGNALED(ret)) {
      printf("Call to system(%s) killed by signal %d\n", name, WTERMSIG(ret));
    } else if (WIFSTOPPED(ret)) {
      printf("Call to system(%s) stopped by signal %d\n", name, WSTOPSIG(ret));
    } else if (WIFCONTINUED(ret)) {
      printf("Call to system(%s) continued\n", name);
    }
}


int main(int argc, char**argv, char**envp) {
    callsys("./test.sh");
    return 0;
}
