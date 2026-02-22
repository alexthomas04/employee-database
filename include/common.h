#include <stdint.h>

#ifndef COMMON_H
#define COMMON_H

#define STATUS_ERROR   -1
#define STATUS_SUCCESS 0
#define PORT 5578

enum protocoltype_e {
  PROTOCOL_HELLO,
  ADD_EMPLOYEE_REQ,
  ADD_EMPLOYEE_RES,
  LIST_EMPLOYEES_REQ,
  LIST_EMPLOYEES_RES,
  WHOAMI_REQ,
  WHOAMI_RES,
  GOODBYE,
};

struct __attribute__((packed)) protocol_t {
  enum protocoltype_e type;
  uint16_t len;
};

struct hello_message_t {
  uint16_t protocolVersion;
};

struct status_response_t {
  int status;
};

struct whoami_response_t {
  char ipString[16];
};



int build_socket_fd_bind(uint32_t s_addr);
int build_socket_fd_connect(uint32_t s_addr);
int read_hello_message(int socket_fd, struct protocol_t protocol);
int read_message(int socket_fd, struct protocol_t protocol, void *out, size_t len);
int write_hello_message(int fd);
int read_protocol(int socket_fd, struct protocol_t *protocol);
int write_protocol_data(int socket_fd, int type, void *data, uint16_t len);

#endif
