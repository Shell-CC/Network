#include <iostream>
#include <fstream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "process.h"
#include "tftp_msg.h"

int main(int argc, char *argv[])
{
  // server info
  struct addrinfo hints, *serv_info;
  int sockfd;
  int rcv;

  // new client info
  struct sockaddr_storage client_addr;
  socklen_t client_addrlen;
  char client_ip[INET6_ADDRSTRLEN];
  unsigned short int client_port;
  int client_sockfd;

  // message info
  char buf[516];
  struct tftp_msg msg;

  // file descripters
  fd_set all_fds, read_fds;
  int fdmax;

  if (argc != 3) {
    std::cout << "Usage: ./server server_ip server_port" << std::endl;
    return -1;
  }

  // get address info of the server
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  rcv = getaddrinfo(argv[1], argv[2], &hints, &serv_info);
  if (rcv != 0) {
    std::cout << "ERROR GETTING ADDRESS INFO: " << std::endl;
    return -1;
  }

  // create a main socket and listen
  sockfd = passive_open(serv_info);
  if (sockfd == -1) return -1;
  // freeaddrinfo(serv_info);
  std::cout << "Server is listening on socket " << sockfd << std::endl;

  FD_ZERO(&all_fds);
  FD_ZERO(&read_fds);
  FD_SET(sockfd, &all_fds);

  while(1) {
    // Select
    read_fds = all_fds;
    if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) == -1) {
      std::cout << "ERROR SELECTING" << std::endl;
      return -1;
    }
    for (int i = 0; i < FD_SETSIZE; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == sockfd) {    // If new connections
          memset(buf, 0, sizeof(buf));
          memset(&client_addr, 0, sizeof(client_addr));
          client_addrlen = sizeof(client_addr);
          if (recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&client_addr,
                       &client_addrlen) == -1) {
            std::cout << "ERROR RECEIVING" << std::endl;
            continue;
          }
          inet_ntop(client_addr.ss_family,
                    get_in_addr((struct sockaddr *)&client_addr),
                    client_ip, sizeof(client_ip));
          client_port = get_in_port((struct sockaddr *)&client_addr);
          memset(&msg, 0, sizeof(msg));
          unpack_msg(buf, &msg);
          switch(msg.opcode) {
          case TFTP_RRQ:
            std::cout << "Read request from " << client_ip << ":" << client_port << std::endl;
            client_sockfd = process_rrq(serv_info, &msg, (struct sockaddr*)&client_addr, client_addrlen);
            if (client_sockfd == -1) {
              close(client_sockfd);
            } else {
              std::cout << "Add " << client_sockfd << std::endl;
              FD_SET(client_sockfd, &read_fds);
            }
            break;
          case TFTP_WRQ:
            std::cout << "Not implemented yet!!!!!!!!!!" << std::endl;
            break;
          default:
            break;
          }
        } else {            // If old connection
          std::cout << "Old connection on " << i << std::endl;
          memset(buf, 0, sizeof(buf));
          memset(&client_addr, 0, sizeof(client_addr));
          client_addrlen = sizeof(client_addr);
          if (recvfrom(i, buf, sizeof(buf), 0, (struct sockaddr*)&client_addr,
                       &client_addrlen) == -1) {
            std::cout << "ERROR RECEIVING" << std::endl;
            continue;
          }
          memset(&msg, 0, sizeof(msg));
          unpack_msg(buf, &msg);
          std::cout << msg.opcode << std::endl;
          switch (msg.opcode) {
          case TFTP_ACK:
            unsigned short blocknum;
            blocknum = unpack_ack(msg.pld);
            std::cout << blocknum << std::endl;
            break;
          default:
            break;
          }
        }
      }
    }
  }
  return 0;
}
