#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define FILE_SIZE 4 * 1024 * 1024 + 1
#define ALLOC_BUFF_ERROR "Error allocate buffer"
#define FILE_NAME "A"

int main(int argc, char *argv[])
{
  int fd = open(FILE_NAME, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);

  char *buffer= (char *)calloc(FILE_SIZE, 1);
  if (buffer == NULL)
  {
    perror(ALLOC_BUFF_ERROR);
    return -1;
  }

  buffer[0] = 1;
  buffer[10000] = 1;
  buffer[FILE_SIZE - 1] = 1;

  write(fd, buffer, FILE_SIZE);
  close(fd);
  free(buffer);
  return 0;
}