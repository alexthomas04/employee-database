#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include "common.h"
#include "parse.h"
#include "file.h"

struct client_t {
  int fd;
  struct sockaddr_in clientAddress;
};

static void ip_to_string(struct client_t *client, char *addressOut){
  uint8_t ip[4] = {0};
  memcpy(ip, &client->clientAddress.sin_addr.s_addr,4);
  snprintf(addressOut, 20, "%d.%d.%d.%d", ip[0],ip[1],ip[2],ip[3]);
}

static int start_server(void) {
  int socket_fd = build_socket_fd_bind(0); // a bound socket on PORT on address 0 (0.0.0.0)
  if(socket_fd == -1) {
    return -1;
  }
  if(listen(socket_fd, 0) == -1){
    perror("listen");
    return -1;
  }
  printf("Started server, listening on %s:%d\n","0.0.0.0",PORT);
  return socket_fd;
}


static int accept_client(int serverFd, struct client_t *client) {
  struct sockaddr_in clientAddress;
  socklen_t clientAddrLen = sizeof(clientAddress);

  int clientSocket = accept(serverFd, (struct sockaddr*)&clientAddress, &clientAddrLen);
  if(clientSocket == -1){
    perror("accept");
    return -1;
  }
  client->fd = clientSocket;
  client->clientAddress = clientAddress;
  char address[20] = {0};
  ip_to_string(client, address);
  printf("Got connection on %s:%d\n", address, clientAddress.sin_port);
  return 0;
}

static int handle_add_employee(struct client_t *client, struct protocol_t protocol, FILE *dbFile, struct dbheader_t *header) {
  if(protocol.len > sizeof(struct employee_t) + 2) {
    printf("Invalid size for employee %d\n", protocol.len);
    return STATUS_ERROR;
  }
  char *buffer = calloc(1, protocol.len);
  if(buffer == NULL){
    printf("Error allocating memory for add employee buffer\n");
    return STATUS_ERROR;
  }
  if(read(client->fd, buffer, protocol.len) != protocol.len) {
    perror("read");
    free(buffer);
    return STATUS_ERROR;
  }
  struct employee_t employee = {0};
  parse_net_employee(buffer, &employee);
  free(buffer);
  if(fseek(dbFile, header->filesize, SEEK_SET) == -1) {
    perror("fseek");
    return STATUS_ERROR;
  }
  if(write_employee(dbFile, &employee) == STATUS_ERROR) {
    printf("Error writing employee to database file\n");
    return STATUS_ERROR;
  }
  header->count++;
  header->filesize+= get_employee_net_size(&employee);
  if(write_header(dbFile, header) == STATUS_ERROR)
    return STATUS_ERROR;
  if(fflush(dbFile) != 0) {
    perror("fflush");
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}


#define CLIENTS 5

static void init_clients(struct client_t *clients) {
  for(int i=0;i<CLIENTS;i++){
    clients[i].fd =-1;
  }
}

static void initialize_client(int server_socket, struct client_t *client) {
  if(accept_client(server_socket,client) == -1) {
    printf("Error connecting to client\n");
    return;
  }
  if(write_hello_message(client->fd) == -1) {
    printf("Error saying hello\n");
    return;
  }
}


static int find_by_fd(struct client_t *clients, int fd) {
  for(int i=0;i<CLIENTS;i++){
    if(clients[i].fd == fd) {
      return i;
    }
  }
  return -1;
}

static int find_empty_client(struct client_t *clients) {
  return find_by_fd(clients, -1);
}

static int handle_client(struct client_t *client, FILE *dbFile, struct dbheader_t *header) {
  struct protocol_t protocol = {0};
  if(read_protocol(client->fd, &protocol) == STATUS_ERROR) {
    printf("Error reading protocol\n");
    return STATUS_ERROR;
  }

  switch (protocol.type) {
    case PROTOCOL_HELLO:
      return read_hello_message(client->fd, protocol)==STATUS_SUCCESS?sizeof(struct hello_message_t):-1;

    case ADD_EMPLOYEE_REQ:
      int status = handle_add_employee(client, protocol, dbFile, header);
      struct status_response_t response = {.status=htonl(status)};
      write_protocol_data(client->fd, ADD_EMPLOYEE_RES, &response, sizeof(response));
      return status;
    case LIST_EMPLOYEES_REQ:
    default:
      printf("Unsupported protocol message\n");
      return STATUS_ERROR;
  }
}

int main(int argc, char *argv[]) {
  int server_socket = start_server();
  if(server_socket == -1) {
    printf("Error starting server\n");
    return -1;
  }
  FILE *dbFile = NULL;
  struct dbheader_t *header = NULL;
  if(open_db_file("mynewdb.db", &dbFile) == STATUS_ERROR) {
    if(create_db_file("mynewdb.db", &dbFile) == STATUS_ERROR) {
      printf("Error opening database file\n");
      return -1;
    }
    if(create_db_header(&header) == STATUS_ERROR){
      printf("Error creating db header\n");
      return -1;
    }
    if(write_header(dbFile, header) == STATUS_ERROR){
      printf("Error writing new db header\n");
      return -1;
    }
  } else if(validate_db_header(dbFile, &header) == -1) {
    printf("Error validating header\n");
    return -1;
  }
  struct client_t clients[CLIENTS] = {0};
  init_clients(clients);
  struct pollfd fds[CLIENTS+1] = {0};
  fds[0].fd = server_socket;
  fds[0].events = POLLIN;
  while(1){
    int pollCount = 1;
    for(int i = 0;i<CLIENTS;i++){
      if(clients[i].fd!=-1){
        fds[pollCount].fd = clients[i].fd;
        fds[pollCount].events = POLLIN;
        pollCount++;
      }
    }
    int eventCount = poll(fds, pollCount, -1);
    if(eventCount < 0) {
      perror("poll");
      return -1;
    }
    if(eventCount>0){
      if(fds[0].revents & POLLIN) {
        int idx = find_empty_client(clients);
        if(idx == -1){
          printf("No Client Slots Available!!!1\n");
        } else {
          initialize_client(server_socket, &clients[idx]);
        }
        eventCount--;
      }
      for(int i = 1; i<pollCount && eventCount>0; i++) {
        if(fds[i].revents & POLLIN) {
          int idx = find_by_fd(clients, fds[i].fd);
          int responseBytes = handle_client(&clients[idx], dbFile, header);
          if( responseBytes < 1) {
            if(responseBytes == -1)
              printf("Error printing responses\n");
            if(close(clients[idx].fd) == -1){
              perror("close");
              printf("Error closing socket %d\n",clients[idx].fd);
            } else {
              printf("Closed socket %d\n",clients[idx].fd);
            }
            clients[idx].fd = -1;
            continue;
          }
          eventCount--;
        }
      }
    }
  }
  return 0;
}
