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

static int send_employee(struct employee_t *employee, int socket_fd) {
  size_t employee_size = get_employee_net_size(employee) - 2; //-2 because the header length is handled by protocol_t.len
  char *buffer = calloc(1, employee_size);
  size_t copiedSize = employee_to_buffer(buffer, employee);
  if( copiedSize!= employee_size){
    printf("Poor allocation of employee buffer: allocated: %d copied: %d\n",employee_size, copiedSize);
    free(buffer);
    return STATUS_ERROR;
  }
  int writeStatus = write_protocol_data(socket_fd, ADD_EMPLOYEE_REQ, buffer, employee_size);
  free(buffer);
  return writeStatus;
}

static int initialize_connection(int socket_fd) {
  if(write_hello_message(socket_fd) != STATUS_SUCCESS) {
    printf("Error sending hello message\n");
    return STATUS_ERROR;
  }

  bool gotVersion = false, gotWhoami = true; //TODO: implement whoami
  
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
    }
  }
  return STATUS_SUCCESS;
}

int main(int argc, char *argv[]) {
  if(argc < 3) {
    printf("Required Arguments: <server ip> <name,address,hours>\n");
    return -1;
  }
  struct employee_t employee = {0};
  if(parse_str_employee(&employee, argv[2]) != STATUS_SUCCESS) {
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
  send_employee(&employee, socket_fd);
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
