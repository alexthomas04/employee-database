#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "file.h"

int create_db_file(char *filename, FILE **fpOut) {
  int fd = open(filename, O_RDONLY);
  if (fd != -1) {
    printf("File \"%s\" already exists\n", filename);
    return STATUS_ERROR;
  }
  *fpOut = fopen(filename, "w+"); // rw with truncate
  if (*fpOut == NULL) {
    perror("fopen");
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

int open_db_file(char *filename, FILE **fpOut) {
  *fpOut = fopen(filename, "r+"); //rw without truncate
  if (*fpOut == NULL) {
    perror("open");
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}
