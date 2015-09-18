/*tcp_process.cpp*/
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) { // ipv4
    return &(((struct sockaddr_in*)sa)->sin_addr);
  } else {  // ipv6
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }
}

unsigned short int get_in_port(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return ntohs(((struct sockaddr_in *)sa)->sin_port);
  } else {
    return ntohs(((struct sockaddr_in6 *)sa)->sin6_port);
  }
}

int process_send(int sockfd, const void *msg, int len, int flags) {
  int nbytes = send(sockfd, msg, len, flags);
  std::cout << "----Sending...";
  if (nbytes == -1){
    std::cout << "failed. Please try again later." << std::endl;
  } else {
    std::cout << "succeed." << std::endl;
  }
  return nbytes;
}


int process_recv(int sockfd, void *buf, int len, int flags) {
  int nbytes = recv(sockfd, buf, len, flags);
  if (nbytes < 0) {
    std::cout << "ERROR RECEIVING" << std::endl;
  } else if (nbytes == 0){
    std::cout << "Connection on socket " << sockfd << " closed."<< std::endl;
  } else {
    std::cout << "----Received from connection on socket " << sockfd << std::endl;
  }
  return nbytes;
}