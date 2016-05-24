#include <unistd.h>
#include <stdio.h>

#include "util.h"

int writePipe(int fd, char msg)
{
  char tmp[3];

  tmp[0] = msg;
  tmp[1] = '\n';
  tmp[2] = '\0';

  //printf("Writing %c to pipe fd: %d\n",msg,fd);

  return write(fd, tmp, 2);
}

int writeFile(char *name, int fd)
{
  FILE *file;
  char buf[255];
  size_t len;
  size_t size = 1;
  size_t max = 255;

  if (NULL != (file = fopen(name, "rb"))) {
    while (0 < (len = fread(buf, size, max, file))) {
      write(fd, buf, len);
    }
    fclose(file);
    return 0;
  }
  return -1;
}
