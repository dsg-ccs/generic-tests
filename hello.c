#include <stdio.h>

int main(int argc, char**argv)
{
  if (argc == 1) {
    printf("x86 ahoy!\n");
  } else {
    printf("x86 hello %s\n",argv[1]);
  }
  return 0;
}
