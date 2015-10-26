#include <iostream>
#include <map>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "process.h"
#include "http_msg.h"

#define MAX_CLIENTS 10

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
  // message info
  char buf[512];
  struct url_req req;
  int rev;
  // file descripters
  fd_set all_fds, read_fds, client_fds;
  int fdmax;
  // lists
  std::map<int, int> cs_fd;

  FD_ZERO(&all_fds);
  FD_ZERO(&read_fds);
  FD_ZERO(&client_fds);
  FD_SET(serv_sockfd, &all_fds);
  fdmax = serv_sockfd;

  while(1) {

    read_fds = all_fds;
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      std::cout << "ERROR SELECTING" << std::endl;
      return -1;
    }
    /**
     * Run through the existing connections looking for data to read.
     */
    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {

        if (i == serv_sockfd) {
          /* handle new connections */
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
          FD_SET(client_sockfd, &client_fds);
          fdmax = client_sockfd > fdmax ? client_sockfd : fdmax;

        } else if (FD_ISSET(i, &client_fds)){
          /* handle receiving a request from a client */
          memset(buf, 0, 512);
          rev = recv(i, buf, sizeof(buf), 0);
          if (rev <= 0) {
            std::cout << "--Connection closed on client " << i << std::endl;
            close(i);
            FD_CLR(i, &client_fds);
            FD_CLR(i, &all_fds);
            continue;
          }
          // parse for domain name and page resource.
          std::cout << "Received request from client on socket " << i << ":" << std::endl;
          if (unpack_header_get(std::string(buf), &req) < 0) {
            std::cout << "ERROR: RECEIVE ILLEGAL MESSAGE" << std::endl;
            continue;
          }
          std::cout << "Domain: " << req.host << std::endl;
          std::cout << "Page: " << req.resc << std::endl;
          // check if in cache
          if (cache_contains(&req)) {
          } else {
            // If not in cache
            std::cout << "Not found in cache, sending to server..." << std::endl;
            // send get to server
            int send_sockfd = send_server_get(&req);
            if (send_sockfd < 0) {
              close(i);
              FD_CLR(i, &all_fds);
              continue;
            }
            cs_fd[send_sockfd] = i;
            FD_SET(send_sockfd, &all_fds);
            fdmax = send_sockfd > fdmax ? send_sockfd : fdmax;
          }
        } else {
          /* handle receiving data from server */
          // recive from server
          string filename = "cache0";
          FILE *fp = fopen(filename.c_str(), "w");
          if (http_recv_write(i, fp) < 0) {
            std::cout << "ERROR RECEIVING: try receiving later" << std::endl;
            fclose(fp);
            continue;
          }
          std::cout << "Save to cache: " << filename << std::endl;
          fclose(fp);
          close(i);
          FD_CLR(i, &all_fds);
          // send to client
          int csockfd = cs_fd[i];
          fp = fopen("cache0", "r");
          proxy_send(csockfd, fp);
          fclose(fp);
          close(csockfd);
          FD_CLR(csockfd, &all_fds);
          FD_CLR(csockfd, &all_fds);
        }
      }
    } // end selection
  }// end listening
  close(serv_sockfd);
  return 0;
}
