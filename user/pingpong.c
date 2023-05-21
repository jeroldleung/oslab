// This is my pingpong.c implementation.

#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char buf[4];

  int p[2];
  pipe(p);

  if(fork() == 0){
    while(!read(p[0], buf, 4)) { } // wait until parent send bytes to child
    write(p[1], "pong", 4);
  } else {
    write(p[1], "ping", 4);
    wait(0);
    read(p[0], buf, 4);
  }

  printf("%d: received %s\n", getpid(), buf); // print to the standard output
  exit(0);
}

