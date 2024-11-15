#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/mount.h>
#include<sys/wait.h>
#include<sched.h>
#include<string.h>
#include<errno.h>

/*
 * This program creates a child in a new uid and chroot
 */

/* Build with 
   gcc -std=gnu99 mkuidns.c -o mkuidns 
*/
static char **sharedenvp;
static int sockets[2];
#define MOUNTQEMU 1
int childfunc(void *arglist) {
  char okmsg[3];
  int ret;
  char **args;
  char *rootdir,*cmd;
  char** cmdargs;
  char bigenoughbuf[2000];
  args = (char **)arglist;
  okmsg[2] = 0;
  read(sockets[0],okmsg,2);
  //printf("Child got ok %s\n",okmsg);
  rootdir = args[0];
  cmd = args[1];
  cmdargs = args+1;
  //printf("rootdir = %s\n",rootdir);
  //printf("cmd = %s\n",cmd);
  #if MOUNTQEMU
  sprintf(bigenoughbuf,"%s/lib64",rootdir);
  ret = mkdir(bigenoughbuf,0555);
  ret = mount("/lib64",bigenoughbuf,NULL,MS_MGC_VAL|MS_BIND|MS_REC|MS_RDONLY,NULL);
  if (ret != 0) {perror("mkuidns: Mount of lib64 failed"); exit(11);}
  sprintf(bigenoughbuf,"%s/qemu",rootdir);
  ret = mkdir(bigenoughbuf,0555);
  ret = mount("/u06/dsg/mountasqemu",bigenoughbuf,NULL,MS_MGC_VAL|MS_BIND|MS_REC|MS_RDONLY,NULL);
  if (ret != 0) {perror("mkuidns: Mount of qemu failed"); exit(12);}
  #endif

  /*
  sprintf(bigenoughbuf,"%s/dev",rootdir);
  ret = mkdir(bigenoughbuf,0555);
  ret = mount("/dev",bigenoughbuf,NULL,MS_MGC_VAL|MS_BIND|MS_REC|MS_RDONLY,NULL);
  if (ret != 0) {perror("mkuidns: Mount of dev failed"); }
  */
  
  sprintf(bigenoughbuf,"%s/proc",rootdir);
  ret = mkdir(bigenoughbuf,0555);
  ret = mount("/proc",bigenoughbuf,NULL,MS_MGC_VAL|MS_BIND|MS_REC|MS_RDONLY,NULL);
  if (ret != 0) {perror("mkuidns: Mount of proc failed"); }

  ret = chroot(rootdir);
  if (ret != 0) {perror("mkuidns: Chroot failed"); exit(22);}
  ret = chdir("/");
  if (ret != 0) {perror("mkuidns: Chdir to / failed"); exit(66);}
  ret = access(cmd,F_OK);
  if (ret != 0) {perror("mkuidns: cmd does not exist"); exit(-2);}
  ret = access(cmd,X_OK);
  if (ret != 0) {perror("mkuidns: No execute access to cmd"); exit(-2);}

  
  //printf("Access to %s for execute is %d\n",cmd,ret);
  //printf("Calling %s with ",cmd);
  for (int i = 0; i < 10; i++) {
    if (cmdargs[i] == NULL) break;
    printf(" %s",cmdargs[i]);
  }
  printf("\n");
  ret = execve(cmd,cmdargs,sharedenvp);
  if (ret != 0) {perror("mkuidns: Execve failed"); exit(-2);}
  return 15; // never get here!!
}

#define STACK_SIZE (1024*1024)
int main(int argc, char**argv, char**envp)
{
    int childpid;
    int ret;

    char bigenoughbuf[2000];
    char *stack,*stacktop;  // memory for child stack, expect to execve soon so do not need much
    setbuf(stdout,NULL);

    // make socket pair to use as sequencer (child waits for parent to set up)
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        perror("opening stream socket pair"); exit(1);}

    stack = malloc(STACK_SIZE);
    if (stack == NULL) {perror("child stack alloc failed"); exit(1);}
    stacktop = stack + STACK_SIZE;  // stack grows down

    sharedenvp = envp;
    
    childpid = clone(childfunc,stacktop,
		     CLONE_NEWIPC|CLONE_NEWNS|CLONE_NEWPID|CLONE_NEWUSER|CLONE_NEWUTS|SIGCHLD,
		     argv+1);
    if (childpid == -1) {perror("clone failed"); exit(1);}
    { // Parent only code
      pid_t cpid;
      int fd;
      int status;
      // Adjust childs uid map
      sprintf(bigenoughbuf,"/proc/%d/uid_map",childpid);
      fd = open(bigenoughbuf,O_WRONLY);
      if (fd == -1) {
	printf("unable to open uid map\n");
	kill(childpid,SIGTERM);
	exit(2);
      }
      sprintf(bigenoughbuf,"0 %06d 1\n",getuid());
      write(fd,bigenoughbuf,11);
      close(fd);
      //printf("Set child %d uid for %d to 0\n%s",childpid,getuid(),bigenoughbuf);
      
      // Adjust set groups
      sprintf(bigenoughbuf,"/proc/%d/setgroups",childpid);
      fd = open(bigenoughbuf,O_WRONLY);
      if (fd == -1) {
	printf("unable to open setgroups\n");
	kill(childpid,SIGTERM);
	exit(2);
      }
      write(fd,"deny\n",5);
      close(fd);
      //printf("Set groups %d to deny\n",childpid);

      // Adjust childs gid map
      sprintf(bigenoughbuf,"/proc/%d/gid_map",childpid);
      fd = open(bigenoughbuf,O_WRONLY);
      if (fd == -1) {
	printf("unable to open gid map\n");
	kill(childpid,SIGTERM);
	exit(2);
      }
      sprintf(bigenoughbuf,"0 %06d 1\n",getgid());
      write(fd,bigenoughbuf,11);
      close(fd);
      //printf("Set child %d gid for %d to 0\n%s",childpid,getgid(),bigenoughbuf);


      // Let child continue
      write(sockets[1],"go",2);

      // Wait for child to complete
      cpid = waitpid(childpid,&status,0);
      if (cpid <= 0) {perror("parent waitpid failed"); exit(1);}
      if (WIFEXITED(status)) {
	if (status != 0) {
	  printf("child exited, status=%d\n", WEXITSTATUS(status));
	}
	// else can be silent
      } else if (WIFSIGNALED(status)) {
	printf("killed by signal %d\n", WTERMSIG(status));
      } else if (WIFSTOPPED(status)) {
	printf("stopped by signal %d\n", WSTOPSIG(status));
      } else if (WIFCONTINUED(status)) {
	printf("continued\n");
      }
      //printf("Parent sees child %d complete with status %d\n",cpid,status);
    } 
}
      

