#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
  int longestName =0, longestAddress=0;
  int i=0;
  for(i=0;i<dbhdr->count;i++){
    struct employee_t e = employees[i];
//    if(strlen(e.name
  }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {

}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {

}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
  struct dbheader_t outputheader = {
    .magic=htonl(dbhdr->magic),
    .version=htons(dbhdr->version),
    .count=htons(dbhdr->count),
    .filesize=htonl(dbhdr->filesize)};
  if(lseek(fd, 0, SEEK_SET) == -1){
    perror("lseek");
    return STATUS_ERROR;
  }
  if(write(fd,&outputheader,sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)){
    perror("write");
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;

}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if(header == NULL){
    printf("Unable to allocate header\n");
    return STATUS_ERROR;
  }
  if(read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)){
    printf("Unable to read header from file\n");
    free(header);
    return STATUS_ERROR;
  }
  //convert network to host endienness 
  header->magic = ntohl(header->magic);
  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->filesize = ntohl(header->filesize);

  if(header->magic != HEADER_MAGIC){
    printf("Invalid database header magic\n");
    free(header);
    return STATUS_ERROR;
  }
  if(header->version != 1){
    printf("Invalid database version number, valid version is: 1\n");
    free(header);
    return STATUS_ERROR;
  }

  struct stat s = {0};
  if(fstat(fd, &s) == -1){
    printf("Error stating database file\n");
    free(header);
    return STATUS_ERROR;
  }
  if(s.st_size != header->filesize){
    printf("Database filesize does not match header\n");
    free(header);
    return STATUS_ERROR;
  }
  *headerOut = header;
  return STATUS_SUCCESS;


}

int create_db_header(int fd, struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1,sizeof(struct dbheader_t));
  if(header == NULL){
    printf("Unable to allocate header\n");
    return STATUS_ERROR;
  }
  header->magic = HEADER_MAGIC;
  header->version = 1;
  header->count = 0;
  header->filesize = sizeof(struct dbheader_t);
  *headerOut = header;
}


