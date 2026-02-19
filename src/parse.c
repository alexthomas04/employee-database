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
  for(;i<dbhdr->count;i++){
    struct employee_t e = employees[i];
    if(strlen(e.name)>longestName)
      longestName = strlen(e.name);
    if(strlen(e.address)>longestAddress)
      longestAddress = strlen(e.address);
  }
  printf("%*s | %*s | Hours\n",longestName,"Name",longestAddress,"Address");
  for(i=0;i<dbhdr->count;i++){
    struct employee_t e = employees[i];
    printf("%*s | %*s | %04d\n",longestName,e.name,longestAddress,e.address,e.hours);
  }

}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
  short idx = dbhdr->count;
  strlcpy(employees[idx].name, strtok(addstring,","), sizeof(employees[idx].name));
  strlcpy(employees[idx].address , strtok(NULL,","), sizeof(employees[idx].address));
  employees[idx].hours = atoi(strtok(NULL,","));
  return STATUS_SUCCESS;
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
  int i =0;
  for(;i<dbhdr->count;i++){
    struct employee_t outputEmployee = {
      .hours=htonl(employees[i].hours)};
    strlcpy(outputEmployee.name, employees[i].name, sizeof(outputEmployee.name));
    strlcpy(outputEmployee.address, employees[i].address, sizeof(outputEmployee.address));
    if(write(fd,&outputEmployee,sizeof(struct employee_t)) != sizeof(struct employee_t)){
      perror("write");
      return STATUS_ERROR;
    }
  }
  return STATUS_SUCCESS;

}	

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
  struct employee_t *employees = calloc(dbhdr->count, sizeof(struct employee_t));
  if(employees == NULL){
    printf("unable to allocate employees\n");
    return STATUS_ERROR;
  }
    if(read(fd,employees, dbhdr->count*sizeof(struct employee_t)) != dbhdr->count*sizeof(struct employee_t)){
      printf("Error reading employees \n");
      free(employees);
      return STATUS_ERROR;
    }
  int i=0;
  for(;i<dbhdr->count;i++){
    employees[i].hours = ntohl(employees[i].hours);
  }
  *employeesOut = employees;
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

int create_db_header(struct dbheader_t **headerOut) {
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


