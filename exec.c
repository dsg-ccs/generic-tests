#include <stdio.h>
#include <unistd.h>
int main(int argc, char **argv, char**envp) {
  printf("Main program %s started with %d args\n",argv[0],argc);
  if (argc == 1) {
    char* argv[] = { "argument1", "argument with spaces", NULL };
    char* envp[] = { "some", "environment", NULL };
    
    if (execve("./hello.static", argv, envp) == -1) {
      printf("Failed to start ./hello.static\n");
      perror("Could not execve");
    }
  } else {
    if (execve(argv[1], argv+1, envp) == -1) {
      printf("Failed to start %s with %d args\n",argv[1], argc);
      perror("Could not execve");
    }
  }
  return 1;
}
