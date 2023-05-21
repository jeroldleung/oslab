// This is my find.c implementation.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

int
foundit(char *path, const char *file)
{
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  int result = strcmp(p, file);
  if(result == 0) { return 1; }
  return 0;
}

void 
find(char *path, const char *file)
{
  char buf[512], *p;
  int fd;
  struct stat st;
  struct dirent de;

  if((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type) {
  case T_DEVICE:

  case T_FILE:
    if(foundit(path, file)) { printf("%s\n", path); }
    break;

  case T_DIR:
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)) {
      if(de.inum == 0 || strcmp(".", de.name) == 0 || strcmp("..", de.name) == 0) { continue; }
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, file);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc < 3) {
    fprintf(2, "Usage: find directory file\n");
    exit(1);
  }

  char *path = argv[1], *file = argv[2];
  find(path, file);

  exit(0);
}

