#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))



static int initialize_connection(int socket_fd) {
  if(write_hello_message(socket_fd) != STATUS_SUCCESS) {
    printf("Error sending hello message\n");
    return STATUS_ERROR;
  }
  if(write_protocol_data(socket_fd, WHOAMI_REQ, NULL, 0) == STATUS_ERROR) {
    printf("Error requesting whoami\n");
    return STATUS_ERROR;
  }

  bool gotVersion = false, gotWhoami = false; 
  
  while(!gotWhoami || !gotVersion) {
    struct protocol_t protocol = {0};
    if(read_protocol(socket_fd, &protocol) == STATUS_ERROR){
      printf("Error reading protocol\n");
      return STATUS_ERROR;
    }
    switch (protocol.type) {
      case PROTOCOL_HELLO:
        if(read_hello_message(socket_fd, protocol) == STATUS_ERROR)
          return STATUS_ERROR;
        gotVersion = true;
        break;
      case WHOAMI_RES:
        struct whoami_response_t response = {0};
        if(read_message(socket_fd, protocol, &response, MIN(protocol.len,sizeof response)) == STATUS_ERROR) {
          printf("Error reading whoami\n");
          return STATUS_ERROR;
        }
        printf("Self Discovery aquired; I am: %s\n", response.ipString);
        gotWhoami = true;
        break;
    }
  }
  return STATUS_SUCCESS;
}

static int c_list_employees(int socket_fd){
  if(write_protocol_data(socket_fd, LIST_EMPLOYEES_REQ, NULL, 0) == STATUS_ERROR){
    printf("Error sending request to get employee list\n");
    return STATUS_ERROR;
  }
  struct protocol_t protocol = {0};
  if(read_protocol(socket_fd, &protocol) == STATUS_ERROR){
    printf("Error reading list employee response protocol\n");
    return STATUS_ERROR;
  }
  if(protocol.type != LIST_EMPLOYEES_RES) {
    printf("Invalid protocol type: %d\n", protocol.type);
    return STATUS_ERROR;
  }
  struct list_employees_response_t response = {0};
  if(read_message(socket_fd, protocol, &response, sizeof response) == STATUS_ERROR) {
    printf("Error reading employee list response\n");
    return STATUS_ERROR;
  }
  response.count = ntohs(response.count);
  struct employee_t *employees = calloc(response.count, sizeof(struct employee_t));
  if(employees == NULL) {
    printf("Error allocating memory for employees\n");
    return STATUS_ERROR;
  }
  printf("Reading %d employees from server\n",response.count);
  for(int i = 0; i < response.count; i++) {
    if(read_protocol(socket_fd, &protocol) == STATUS_ERROR){
      printf("Error reading send employee protocol\n");
      return STATUS_ERROR;
    }
    if(protocol.type != SEND_EMPLOYEE) {
      printf("Invalid protocol type: %d\n", protocol.type);
      return STATUS_ERROR;
    }
    if(recieve_employee(socket_fd, protocol, &employees[i]) == STATUS_ERROR){
      printf("Error reading employee #%d\n",i);
      return STATUS_ERROR;
    }
  }
  struct dbheader_t dummyHeader = {0};
  dummyHeader.count = response.count;
  list_employees(&dummyHeader, employees);
  return STATUS_SUCCESS;

}

static int c_add_employee(int socket_fd, char *employeeStr) {
  struct employee_t employee = {0};
  if(parse_str_employee(&employee, employeeStr) != STATUS_SUCCESS) {
    return -1;
  }
  send_employee(&employee, socket_fd, ADD_EMPLOYEE_REQ);
  struct protocol_t protocol = {0};
  if(read_protocol(socket_fd, &protocol) == STATUS_ERROR) { 
    return -1;
  }
  if(protocol.type != ADD_EMPLOYEE_RES) {
    printf("Unexpected protocol type %d expected %d\n",protocol.type,ADD_EMPLOYEE_RES);
    return -1;
  }
  struct status_response_t response = {0};
  if(read_message(socket_fd, protocol, &response, sizeof(response)) == STATUS_ERROR) {
    printf("Error reading add employee response\n");
    return -1;
  }
  printf("Got %s response from add employee\n",response.status==STATUS_SUCCESS?"successful":"error");
  
  return 0;
}

int main(int argc, char *argv[]) {
  if(argc < 2) {
    printf("Required Arguments: <server ip>\n");
    return -1;
  }
  struct in_addr addr = {0};
  if(inet_aton(argv[1], &addr) == 0){
    printf("Error parsing address %s\n",argv[1]);
    return -1;
  }
  int socket_fd = build_socket_fd_connect(addr.s_addr);
  if(socket_fd == -1)
    return -1;
  if(initialize_connection(socket_fd) == STATUS_ERROR) {
    printf("Error initializing connection\n");
    return -1;
  }
  int status = 0;
  if(argc == 2) {
    status = c_list_employees(socket_fd);
  } else {
  status = c_add_employee(socket_fd, argv[2]);
  }
  if(write_protocol_data(socket_fd, GOODBYE, NULL, 0) == STATUS_ERROR) {
    printf("Error saying goodbye\n");
    return -1;
  }
  return status;
}
