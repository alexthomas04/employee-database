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

int parse_str_employee(struct employee_t *employee,
    char *addstring) {
  char *name = strtok(addstring, ","),
       *address = strtok(NULL,","),
       *hours = strtok(NULL,",");
  if(name == NULL || address == NULL || hours == NULL){
    printf("Invalid add input, required format is \"<name>,<address>,<hours>\" as a comma separated list\n");
    return STATUS_ERROR;
  }
  strlcpy(employee->name,name , sizeof(employee->name));
  strlcpy(employee->address, address, sizeof(employee->address));
  employee->hours = atoi(hours);
  return STATUS_SUCCESS;
}

int get_employee_net_size(struct employee_t *employee) {
  int size = 8; // 2 (row header/record length) + 4 (hours) + 1x2 (name/address lengths)
  size += strnlen(employee->name, sizeof employee->name);
  size += strnlen(employee->address, sizeof employee->address);
  return size;
}

int output_file(FILE *fp, struct dbheader_t *dbhdr,
    struct employee_t *employees) {
  if(write_header(fp, dbhdr) == STATUS_ERROR) {
    printf("Error writing header\n");
    return STATUS_ERROR;
  }
  for (int i = 0; i < dbhdr->count; i++) {
    if (write_employee(fp, &employees[i]) == STATUS_ERROR) {
      return STATUS_ERROR;
    }
  }
  return STATUS_SUCCESS;
}

int write_header(FILE *fp,struct dbheader_t *dbhdr){
  struct dbheader_t outputheader = {.magic = htonl(dbhdr->magic),
    .version = htons(dbhdr->version),
    .count = htons(dbhdr->count),
    .filesize = htonl(dbhdr->filesize)};
  if (fseek(fp, 0, SEEK_SET) == -1) {
    perror("fseek");
    return STATUS_ERROR;
  }
  if (fwrite(&outputheader, sizeof(struct dbheader_t), 1, fp) != 1) {
    perror("fwrite");
    return STATUS_ERROR;
  }
  if(fflush(fp) != 0) {
    perror("fflush");
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

int write_employee(FILE *fp, struct employee_t *e) {
  uint8_t *buffer = malloc(sizeof(struct employee_t)+2);
  if (buffer == NULL) {
    printf("Unable to allocate buffer to write employee\n");
    return STATUS_ERROR;
  }
  size_t payload_length = employee_to_buffer(&buffer[2], e);
  if(payload_length == STATUS_ERROR){
    free(buffer);
    return STATUS_ERROR;
  }
  uint16_t total_length = payload_length + 2;
  uint16_t net_len = htons(payload_length);
  memcpy(buffer, &net_len, 2);
  size_t items_written = fwrite(buffer, total_length, 1, fp);
  free(buffer);
  return 1 == items_written ? STATUS_SUCCESS : STATUS_ERROR;
}

size_t employee_to_buffer(char *buffer, struct employee_t *e) {
  size_t offset = 0;
  size_t name_len = strnlen(e->name, sizeof e->name);
  size_t address_len = strnlen(e->address, sizeof e->address);
  if (name_len > 255 || address_len > 255) {
    printf("Invalid name or address length, max length is 255\n");
    return STATUS_ERROR;
  }
  buffer[offset++] = (uint8_t)name_len;
  buffer[offset++] = (uint8_t)address_len;

  memcpy(buffer + offset, &e->name, name_len);
  offset += name_len;
  memcpy(buffer + offset, &e->address, address_len);
  offset += address_len;

  uint32_t net_hours = htonl(e->hours);
  memcpy(buffer + offset, &net_hours, 4);
  offset += 4;
  return offset;
}

int read_employees(FILE *fp, struct dbheader_t *dbhdr,
    struct employee_t **employeesOut) {
  struct employee_t *employees =
    calloc(dbhdr->count, sizeof(struct employee_t));
  if (employees == NULL) {
    printf("unable to allocate employees\n");
    return STATUS_ERROR;
  }
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
    parse_net_employee(buffer, &employees[i]);
  }
  free(buffer);
  *employeesOut = employees;
  return STATUS_SUCCESS;
}

void parse_net_employee(char *buffer, struct employee_t *employee) {
  size_t offset = 0;
  size_t name_len = (size_t)buffer[offset++];
  size_t address_len = (size_t)buffer[offset++];
  strlcpy(employee->name, buffer + offset, name_len + 1); // +1 for null terminator
  employee->name[name_len] = '\0';
  offset += name_len;
  strlcpy(employee->address, buffer + offset, address_len + 1);
  employee->address[address_len] = '\0';
  offset += address_len;
  memcpy(&employee->hours, buffer + offset, 4);
  employee->hours = ntohl(employee->hours);
}

int validate_db_header(FILE *fp, struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    printf("Unable to allocate header\n");
    return STATUS_ERROR;
  }
  if (fread( header, sizeof(struct dbheader_t), 1, fp) != 1) {
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
  int fd = fileno(fp);
  if(fd == -1){
    perror("fileno");
    free(header);
    return STATUS_ERROR;
  }
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
