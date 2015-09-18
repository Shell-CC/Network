#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "bcp_process.h"

int main(int argc, char *argv[])
{
  // client info
  struct addrinfo hints, *serv_info;
  char* username;

  if (argc != 4) {
    std::cout << "Usage: ./client username server_ip server_port" << std::endl;
    return -1;
  }
  int client_sockfd;
  int status;
  // server info
  char serv_ip[INET6_ADDRSTRLEN];
  unsigned short int serv_port;

  /**
   * Get address info of the server.
   */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(argv[2], argv[3], &hints, &serv_info);
  if (status != 0) {
    std::cout << "ERROR GETTING ADDRESS INFO: " << gai_strerror(status) << std::endl;
    return -1;
  }
  /**
   * Create a socket for the server.
   */
  client_sockfd = socket(serv_info->ai_family, serv_info->ai_socktype, serv_info->ai_protocol);
  if (client_sockfd < 0) {
    std::cout << "ERROR CREATING SOCKET" << std::endl;
    return -1;
  }
  std::cout << "Created a socket with the below parameters: " << std::endl;
  /**
   * Connect to the server.
   */
  status = connect(client_sockfd, serv_info->ai_addr, serv_info->ai_addrlen);
  if (status < 0) {
    std::cout << "ERROR CONNECTING: " << strerror(errno) << std::endl;
    return -1;
  }
  inet_ntop(serv_info->ai_family, get_in_addr(serv_info->ai_addr), serv_ip, sizeof(serv_ip));
  serv_port = get_in_port(serv_info->ai_addr);
  std::cout << "Connected to the server " << serv_ip << ":" << serv_port << std::endl;

  /**
   * Use select to handle both seding and receiving message.
   */
  char read_buf[512];
  char write_buf[512];
  fd_set all_fds, read_fds;

  FD_ZERO(&all_fds);
  FD_ZERO(&read_fds);
  FD_SET(client_sockfd, &all_fds);
  FD_SET(STDIN_FILENO, &all_fds);

  setbuf(stdout, NULL);
  setbuf(stdin, NULL);
  while(1) {
    std::cout << ">> ";
    read_fds = all_fds;
    fflush(stdout);
    fflush(stdin);
    if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) == -1) {
      std::cout << "SELECTING ERROR" << std::endl;
      return -1;
    }
    int nbytes;
    for (int i = 0; i < FD_SETSIZE; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == STDIN_FILENO) {    // Handleing sending message.
          nbytes = read(0, read_buf, sizeof(read_buf));
          process_send(client_sockfd, read_buf, nbytes, 0);
        } else if (i == client_sockfd){ // Handling receiving message.
          nbytes = process_recv(i, write_buf, sizeof(write_buf), 0);
          if (nbytes > 0) {
            std::cout << "<< " << write_buf << std::endl;
          }
        }
      }
    }
  }

  return 0;
}
