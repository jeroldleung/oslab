// This is my xargs.c implementation.

#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
  if(argc < 2) {
    fprintf(2, "Usage: xargs command [argument ...]\n");
    exit(1);
  }

  char *cmd = argv[1];
  char *args[MAXARG];
  int count;
  for(count = 0; count < argc - 1; count++) {
    args[count] = argv[count+1]; // drop argv[0] which is "xargs"
  }

  // read from standard input and store in args
  char buf[512], ch;
  char *p = buf;
  while(read(0, &ch, 1)) {
    if(ch == '\n') {
      *p = '\0';
      args[count++] = buf;
      p = buf;
    } else {
      *p++ = ch;
    }
  }

  if(fork() == 0) {
    exec(cmd, args);
    fprintf(2, "exec failed!\n");
    exit(1);
  } else {
    wait(0);
  }

  exit(0);
}

