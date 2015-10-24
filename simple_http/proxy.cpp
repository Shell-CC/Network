#include <iostream>
#include <map>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
// #include "proxy_process.h"

define MAX_CLIENTS 10

int main(int argc, char *argv[])
{
  // server info
  struct addrinfo hints, *serv_info;
  char serv_ip[INET6_ADDRSTRLEN];
  unsigned short serv_port;

  int serv_sockfd;
  int status;

  if (argc != 3) {
    std::cout << "Usage: ./proxy proxy_ip proxy_port" << std::endl;
    return -1;
  }

  /* get address info of the server, create socket, bind and listen*/
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(argv[1], argv[2], &hints, &serv_info);
  max_clients = atoi(argv[3]);
  if (status != 0) {
    std::cout << "ERROR GETTING ADDRESS INFO: " << gai_strerror(status) << std::endl;
    return -1;
  }
  serv_sockfd = socket(serv_info->ai_family, serv_info->ai_socktype, serv_info->ai_protocol);
  if (serv_sockfd < 0) {
    std::cout << "ERROR CREATING SOCKET" << std::endl;
    return -1;
  }
  status = bind(serv_sockfd, serv_info->ai_addr, serv_info->ai_addrlen);
  if (status < 0) {
    std::cout << "ERROR BINDING" << std::endl;
    return -1;
  }
  inet_ntop(serv_info->ai_family, get_in_addr(serv_info->ai_addr), serv_ip, sizeof(serv_ip));
  serv_port = get_in_port(serv_info->ai_addr);
  std::cout << "Proxy is now open on " << serv_ip << ":" << serv_port << std::endl;
  freeaddrinfo(serv_info);
  status = listen(serv_sockfd, MAX_CLIENTS);
  if (status < 0) {
    std::cout << "ERROR LISTENING: " << std::endl;
    return -1;
  }
  std::cout << "Listening..." << std::endl;

  // new client info
  struct sockaddr_storage client_addr;
  socklen_t client_addrlen;
  char client_ip[INET6_ADDRSTRLEN];
  unsigned short int client_port;
  int client_sockfd;

  // file descripters
  fd_set all_fds, read_fds;
  int fdmax;

  FD_ZERO(&all_fds);
  FD_ZERO(&read_fds);
  FD_SET(serv_sockfd, &all_fds);
  fdmax = serv_sockfd;

  while(1) {

    read_fds = all_fds;

    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      std::cout << "ERROR SELECTING" << std::endl;
      return -1;
    }
    /**
     * Run through the existing connections lonking for data to read.
     */
    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == serv_sockfd) {  // handle new connections
          client_addrlen = sizeof(client_addr);
          client_sockfd = accept(serv_sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
          if (client_sockfd < 0) {
            std::cout << "ERROR CONNECTING" << std::endl;
            continue;
          }
          inet_ntop(client_addr.ss_family,
                    get_in_addr((struct sockaddr *)&client_addr),
                    client_ip, sizeof(client_ip));
          client_port = get_in_port((struct sockaddr *)&client_addr);
          std::cout << "--New connection from " << client_ip << ":" << client_port
                    << " on socket " << client_sockfd << std::endl;
          FD_SET(client_sockfd, &all_fds);
        } else {  // receive data from a client
        }
      }
    } // end selection
  }// end listening
  close(serv_sockfd);
  return 0;
}
