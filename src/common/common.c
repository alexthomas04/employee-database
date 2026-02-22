#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"

static int make_socket_and_addr(uint32_t s_addr, struct sockaddr_in* out) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return -1;

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        return -1;

    out->sin_port = htons(PORT);
    out->sin_family = AF_INET;
    out->sin_addr.s_addr = s_addr;

    return fd;
}

int build_socket_fd_bind(uint32_t s_addr) {
    struct sockaddr_in info = {0};
    int fd = make_socket_and_addr(s_addr, &info);
    if (fd < 0) return -1;
    if(bind(fd, (struct sockaddr*)&info, sizeof(info)) == -1){
      perror("bind");
      return -1;
    }
    return fd;
}

int build_socket_fd_connect(uint32_t s_addr) {
    struct sockaddr_in info = {0};
    int fd = make_socket_and_addr(s_addr, &info);
    if (fd < 0) return -1;
    if(connect(fd, (struct sockaddr*)&info, sizeof(info)) == -1){
      perror("connect");
      return -1;
    }
    return fd;
}

int read_message(int socket_fd, struct protocol_t protocol, void *out, size_t len) {
  if(protocol.len != len){
    printf("invalid version length: %d\n",protocol.len);
    return STATUS_ERROR;
  }
  if(read(socket_fd, out, len) == -1){
    perror("read");
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

int read_hello_message(int socket_fd, struct protocol_t protocol) {
  struct hello_message_t helloMessage = {0};
  if(read_message(socket_fd, protocol, &helloMessage, sizeof(struct hello_message_t)) == STATUS_ERROR) {
    printf("Error reading hello message\n");
    return STATUS_ERROR;
  }

  uint16_t versionNumber = ntohs(helloMessage.protocolVersion);
  printf("Client connected with v%d\n", versionNumber);
  //TODO: check protocol version and error on invalid protocols
  return STATUS_SUCCESS;
}


int write_hello_message(int fd) {
  struct hello_message_t helloMessage = {.protocolVersion=htons(1)};
  return write_protocol_data(fd, PROTOCOL_HELLO, &helloMessage, sizeof(struct hello_message_t));
}

int read_protocol(int socket_fd, struct protocol_t *protocol) {
  size_t protocolSize = sizeof(struct protocol_t);
  size_t readLen = 0;
    readLen = read(socket_fd, protocol, protocolSize);
    if(readLen == -1){
      perror("read");
      return STATUS_ERROR;
    } 
    if(readLen != protocolSize){
      printf("Malformed protocol\n");
      return STATUS_ERROR;
    }
    protocol->len = ntohs(protocol->len);
    protocol->type = ntohl(protocol->type);
    return STATUS_SUCCESS;
}

int write_protocol_data(int socket_fd, int type, void *data, uint16_t len) {
  size_t totalSize = sizeof(struct protocol_t) + len;
  struct protocol_t protocol = {.type=htonl(type), .len=htons(len)};
  char *buffer = calloc(1, totalSize);
  if(buffer == NULL) {
    printf("Error allocating buffer to write protocol data\n");
    return STATUS_ERROR;
  }
  memcpy(buffer, &protocol, sizeof(struct protocol_t));
  memcpy(&buffer[sizeof(struct protocol_t)], data, len);
  if(write(socket_fd, buffer, totalSize) != totalSize){
    perror("write");
    free(buffer);
    return STATUS_ERROR;
  }
  free(buffer);
  return STATUS_SUCCESS;
}

