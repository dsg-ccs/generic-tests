#include <stdio.h>

int main(int argc, char**argv)
{
  char ch;
  FILE* file=stdout; 
  printf("Will wait for char to be entered\n");
  fflush(file);
  ch = getchar();
  if (ch == EOF) {
    printf("Interrupted\n");
  } else {
    printf("Got %c\n",ch);
  }
  return 0;
}
