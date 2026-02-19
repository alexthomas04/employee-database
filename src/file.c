#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "file.h"

int create_db_file(char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd != -1) {
    printf("File \"%s\" already exists\n", filename);
    return STATUS_ERROR;
  }
  fd = open(filename, O_CREAT | O_RDWR, 0664);
  if (fd == -1) {
    perror("open");
    return STATUS_ERROR;
  }
  return fd;
}

int open_db_file(char *filename) {
  int fd = open(filename, O_RDWR, 0644);
  if (fd == -1) {
    perror("open");
    return STATUS_ERROR;
  }
  return fd;
}
