#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/wait.h>
#include<string.h>
#include<errno.h>
#include <sys/utsname.h>


/*
 * This program gets various identifing info a prints it
 */

int main(int argc, char**argv, char**envp)
{
  struct utsname uts;

  int ret;
  setbuf(stdout,NULL);

  ret = uname(&uts);
  printf("%s %s %s %s %s\n", uts.sysname, uts.nodename, uts.release, uts.version, uts.machine);

  printf("%d 0x%x %d 0x%x\n",getuid(),getuid(),geteuid(),geteuid());

  printf("%d 0x%x %d 0x%x\n",getgid(),getgid(),getegid(),getegid());
}
