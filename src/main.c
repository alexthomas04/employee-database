#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
  printf("%s [flags]\n", argv[0]);
  printf("\t-n Creates a new file\n");
  printf("\t-f <filepath> (required) The filepath of the database\n");
  printf("\t-h Shows this message\n");
}

int main(int argc, char *argv[]) {
  char *filepath = NULL;
  char *addEmployee = NULL;
  bool newfile = false;
  bool listEmployees = false;

  int c = 0;
  while ((c = getopt(argc, argv, "nf:ha:l")) != -1) {
    switch (c) {
    case 'n':
      newfile = true;
      break;
    case 'f':
      filepath = optarg;
      break;
    case 'h':
      print_usage(argv);
      return 0;
    case 'a':
      addEmployee = optarg;
      break;
    case 'l':
      listEmployees = true;
      break;
    case '?':
      printf("Unknown option-%c\n", c);
      break;
    default:
      return -1;
    }
  }
  if (filepath == NULL) {
    printf("Filepath is required\n");
    print_usage(argv);
    return -1;
  }
  int database_fd = newfile ? create_db_file(filepath) : open_db_file(filepath);
  if (database_fd == STATUS_ERROR) {
    printf("Unable to access database file\n");
    return -1;
  }
  struct dbheader_t *header = NULL;
  if ((newfile ? create_db_header(&header)
               : validate_db_header(database_fd, &header)) == STATUS_ERROR) {
    printf("Error processing database header\n");
    return -1;
  }
  if (header == NULL) {
    printf("Error, header is null\n");
    return -1;
  }
  struct employee_t *employees = NULL;
  if (read_employees(database_fd, header, &employees) == STATUS_ERROR) {
    printf("Error reading employees\n");
    return -1;
  }
  if (addEmployee != NULL) {
    employees =
        realloc(employees, sizeof(struct employee_t) * (header->count + 1));
    if (employees == NULL) {
      printf("Error reallocating empoyees for new employee\n");
      return -1;
    }
    add_employee(header, employees, addEmployee);
    header->filesize += get_employee_disk_size(&employees[header->count]);
    header->count = header->count + 1;
  }
  if (listEmployees)
    list_employees(header, employees);
  if (output_file(database_fd, header, employees) == STATUS_ERROR) {
    printf("Error saving database to file\n");
    return -1;
  }
  free(header);
  free(employees);
  return 0;
}
