#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "tcp_process.h"

#define SERVER_PORT 8888
#define BACKLOG 2

int main(int argc, char *argv[])
{
  int server_socket, client_socket;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  int err;

  // Create a socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    printf("socket error\n");
    return -1;
  }

  // build address data structure
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(SERVER_PORT);

  // passive open
  err = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (err < 0) {
    printf("bind error\n");
    return -1;
  }
  err = listen(server_socket, BACKLOG);
  if (err < 0) {
    printf("listen error\n");
    return -1;
  }

  // waiting for connection
  while(1) {
    socklen_t addrlen = sizeof(struct sockaddr);
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addrlen);
    if (client_socket < 0) {
      continue;
    }

    pid_t pid = fork();
    if (pid == 0) {
      process_conn_server(client_socket);
      close(server_socket);
    } else {
      close(client_socket);
    }
  }
}



