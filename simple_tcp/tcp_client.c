#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "tcp_process.h"

#define PORT 8888

int main(int argc, char *argv[])
{
  int s;
  struct sockaddr_in server_addr;

  // create a socket
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    printf("socket error\n");
    return -1;
  }

  // build address data structure
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(PORT);

  inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

  connect(s, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));

  process_conn_client(s);

  close(s);
  return 0;
}
