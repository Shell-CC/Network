#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "http_msg.h"
#include "process.h"

int main(int argc, char *argv[])
{
  // client info
  struct addrinfo hints, *serv_info;
  int client_sockfd;
  int status;
  // proxy info
  char serv_ip[INET6_ADDRSTRLEN];
  unsigned short int serv_port;
  // server info
  string url;
  // message info
  int nbytes;
  char read_buf[512];
  char write_buf[512];

  if (argc != 4) {
    std::cout << "Usage: ./client proxy_ip proxy_port url_to_retrieve" << std::endl;
    return -1;
  }

  /* Get address info of the proxy, create socket and connect*/
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(argv[1], argv[2], &hints, &serv_info);
  if (status != 0) {
    std::cout << "ERROR GETTING ADDRESS INFO: " << std::endl;
    return -1;
  }
  client_sockfd = socket(serv_info->ai_family, serv_info->ai_socktype, serv_info->ai_protocol);
  if (client_sockfd < 0) {
    std::cout << "ERROR CREATING SOCKET" << std::endl;
    return -1;
  }
  status = connect(client_sockfd, serv_info->ai_addr, serv_info->ai_addrlen);
  if (status < 0) {
    std::cout << "ERROR CONNECTING: " << std::endl;
    return -1;
  }
  inet_ntop(serv_info->ai_family, get_in_addr(serv_info->ai_addr), serv_ip, sizeof(serv_ip));
  serv_port = get_in_port(serv_info->ai_addr);
  std::cout << "You have connected to the proxy " << serv_ip << ":" << serv_port << std::endl;

  /* Send get request to proxy server */
  url = string(argv[3]);
  string msg = pack_header_get(url);
  size_t len = msg.length();
  std::cout << "Sending request...\n" << msg;
  if (send(client_sockfd, msg.c_str(), len, 0) < 0) {
    cout << "  ERROR SENDING" << endl;
  }
  std::cout << "Success!" << std::endl;

  /* Receive the file from proxy server */
  string filename = "client_recv";
  FILE *fp = fopen(filename.c_str(), "w");
  if (http_recv_write(client_sockfd, fp) < 0) {
    std::cout << "ERROR RECEIVING" << std::endl;
  }
  std::cout << "Saved to file: " << filename << std::endl;
  fclose(fp);

  close(client_sockfd);
  return 0;
}
