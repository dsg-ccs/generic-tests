#include <termios.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char**argv, char**envp)
{
  int ret;
  struct termios termres;
  ret = tcgetattr(1,&termres);
}
