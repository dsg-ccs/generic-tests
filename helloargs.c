#include <stdio.h>
#include <stdlib.h>
#define MAXENV 3
int main(int argc, char**argv, char**envp)
{
  char **ep;
  int maxenv=MAXENV;
  if (argc == 1) {
    printf("ahoy!\n");
  } else {
    printf("%s had args:",argv[0]);
    for (int i=1; i<argc; i++) {
      printf("| %s|",argv[i]);
    }
    printf("\n");

    if (argc > 1) {
      maxenv = atoi(argv[1]);
    }
    int index = 0;
    while( envp[ index ] ) 
      { // OK to assume null
	// print one environment variable at a time
	if (index <= maxenv) {
	  printf( "envp[%d] = \"%s\"\n", index, envp[ index ] );
	}
	index++;
      } //end while
    printf( "Number of environment vars= %d\n", index );

      
  }
  return 0;
}
