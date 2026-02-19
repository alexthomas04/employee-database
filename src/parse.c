#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
  int longestName = 0, longestAddress = 0;
  int i = 0;
  for (; i < dbhdr->count; i++) {
    struct employee_t e = employees[i];
    if (strlen(e.name) > longestName)
      longestName = strlen(e.name);
    if (strlen(e.address) > longestAddress)
      longestAddress = strlen(e.address);
  }
  printf("%*s | %*s | Hours\n", longestName, "Name", longestAddress, "Address");
  for (i = 0; i < dbhdr->count; i++) {
    struct employee_t e = employees[i];
    printf("%*s | %*s | %04d\n", longestName, e.name, longestAddress, e.address,
        e.hours);
  }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees,
    char *addstring) {
  short idx = dbhdr->count;
  char *name = strtok(addstring, ","),
       *address = strtok(NULL,","),
       *hours = strtok(NULL,",");
  if(name == NULL || address == NULL || hours == NULL){
    printf("Invalid add input, required format is \"<name>,<address>,<hours>\" as a comma separated list\n");
    return STATUS_ERROR;
  }
  strlcpy(employees[idx].name,name , sizeof(employees[idx].name));
  strlcpy(employees[idx].address, address, sizeof(employees[idx].address));
  employees[idx].hours = atoi(hours);
  return STATUS_SUCCESS;
}

int get_employee_disk_size(struct employee_t *employee) {
  int size = 8; // 2 (row header/record length) + 4 (hours) + 1x2 (name/address lengths)
  size += strnlen(employee->name, sizeof employee->name);
  size += strnlen(employee->address, sizeof employee->address);
  return size;
}

// open `FILE` with fdopen
int output_file(int fd, struct dbheader_t *dbhdr,
    struct employee_t *employees) {
  struct dbheader_t outputheader = {.magic = htonl(dbhdr->magic),
    .version = htons(dbhdr->version),
    .count = htons(dbhdr->count),
    .filesize = htonl(dbhdr->filesize)};
  if (lseek(fd, 0, SEEK_SET) == -1) {
    perror("lseek");
    return STATUS_ERROR;
  }
  FILE *fp = fdopen(fd, "w");
  if (fwrite(&outputheader, sizeof(struct dbheader_t), 1, fp) != 1) {
    perror("fwrite");
    return STATUS_ERROR;
  }
  int i = 0;
  for (; i < dbhdr->count; i++) {
    if (write_employee(fp, &employees[i]) == STATUS_ERROR) {
      return STATUS_ERROR;
    }
  }
  return STATUS_SUCCESS;
}

int write_employee(FILE *fp, struct employee_t *e) {
  size_t name_len = strnlen(e->name, sizeof e->name);
  size_t address_len = strnlen(e->address, sizeof e->address);

  if (name_len > 255 || address_len > 255) {
    printf("Invalid name or address length, max length is 255\n");
    return STATUS_ERROR;
  }

  // length of lengths (2) + name length + address length + hours length (4)
  uint16_t payload_length = (uint16_t)(2 + name_len + address_len + 4);

  // total record bytes including the 2-byte record length field
  uint16_t total_length = payload_length + 2;

  uint8_t *buffer = malloc(total_length);
  if (buffer == NULL) {
    printf("Unable to allocate buffer to write employee\n");
    return STATUS_ERROR;
  }
  size_t offset = 0;
  uint16_t net_len = htons(payload_length);
  memcpy(buffer + offset, &net_len, 2);
  offset += 2;
  buffer[offset++] = (uint8_t)name_len;
  buffer[offset++] = (uint8_t)address_len;

  memcpy(buffer + offset, &e->name, name_len);
  offset += name_len;
  memcpy(buffer + offset, &e->address, address_len);
  offset += address_len;

  uint32_t net_hours = htonl(e->hours);
  memcpy(buffer + offset, &net_hours, 4);
  offset += 4;

  size_t items_written = fwrite(buffer, total_length, 1, fp);
  free(buffer);
  return 1 == items_written ? STATUS_SUCCESS : STATUS_ERROR;
}

int read_employees(int fd, struct dbheader_t *dbhdr,
    struct employee_t **employeesOut) {
  struct employee_t *employees =
    calloc(dbhdr->count, sizeof(struct employee_t));
  if (employees == NULL) {
    printf("unable to allocate employees\n");
    return STATUS_ERROR;
  }
  FILE *fp = fdopen(fd, "r");
  // we know that the max length of the buffer is the length of the employee
  // plus the size of the strings, so we can safely allocate it once and just
  // use the length header to know what parts to use right?
  size_t max_size = sizeof(struct employee_t) + 2;
  uint8_t *buffer = malloc(max_size);
  if (buffer == NULL) {
    printf("Unable to allocate buffer for reading employees\n");
    free(employees);
    return STATUS_ERROR;
  }
  int i = 0;
  for (; i < dbhdr->count; i++) {
    uint16_t buffer_length = 0;
    if (fread(&buffer_length, 2, 1, fp) != 1) {
      printf("Unable to read buffer length\n");
      free(employees);
      return STATUS_ERROR;
    }
    buffer_length = ntohs(buffer_length);
    if(buffer_length > max_size){
      printf("Invalid buffer size %d\n",buffer_length);
      free(buffer);
      free(employees);
      return STATUS_ERROR;
    }
    if (fread(buffer, buffer_length, 1, fp) != 1) {
      perror("fread");
      printf("Unable to read employee to buffer %d\n", buffer_length);
      free(buffer);
      free(employees);
      return STATUS_ERROR;
    }
    size_t offset = 0;
    size_t name_len = (size_t)buffer[offset++];
    size_t address_len = (size_t)buffer[offset++];
    strlcpy(employees[i].name, buffer + offset,
        name_len + 1); // +1 for null terminator
    offset += name_len;
    strlcpy(employees[i].address, buffer + offset, address_len + 1);
    offset += address_len;
    memcpy(&employees[i].hours, buffer + offset, 4);
    employees[i].hours = ntohl(employees[i].hours);
  }
  free(buffer);
  *employeesOut = employees;
  return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    printf("Unable to allocate header\n");
    return STATUS_ERROR;
  }
  if (read(fd, header, sizeof(struct dbheader_t)) !=
      sizeof(struct dbheader_t)) {
    printf("Unable to read header from file\n");
    free(header);
    return STATUS_ERROR;
  }
  // convert network to host endianness
  header->magic = ntohl(header->magic);
  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->filesize = ntohl(header->filesize);

  if (header->magic != HEADER_MAGIC) {
    printf("Invalid database header magic\n");
    free(header);
    return STATUS_ERROR;
  }
  if (header->version != 1) {
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
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    printf("Unable to allocate header\n");
    return STATUS_ERROR;
  }
  header->magic = HEADER_MAGIC;
  header->version = 1;
  header->count = 0;
  header->filesize = sizeof(struct dbheader_t);
  *headerOut = header;
  return STATUS_SUCCESS;
}
