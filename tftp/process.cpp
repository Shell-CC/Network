/*tcp_process.cpp*/
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "tftp_msg.h"

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

void change_in_port(struct sockaddr *sa, unsigned short port) {
  if (sa->sa_family == AF_INET) {
    ((struct sockaddr_in *)sa)->sin_port = htons(port);
  } else {
    ((struct sockaddr_in6 *)sa)->sin6_port = htons(port);
  }
}

int passive_open(struct addrinfo *serv_info) {
  char serv_ip[INET6_ADDRSTRLEN];
  unsigned short int serv_port;
  // Create a socket for the server
  int sockfd = socket(serv_info->ai_family, serv_info->ai_socktype, serv_info->ai_protocol);
  if (sockfd < 0) {
    std::cout << "ERROR CREATING SOCKET" << std::endl;
    return -1;
  }
  // Passive open: bind and listen.
  int rcv = bind(sockfd, serv_info->ai_addr, serv_info->ai_addrlen);
  if (rcv < 0) {
    std::cout << "ERROR BINDING: " << std::endl;
    return -1;
  }

  getsockname(sockfd, serv_info->ai_addr, &serv_info->ai_addrlen);
  inet_ntop(serv_info->ai_family, get_in_addr(serv_info->ai_addr), serv_ip, sizeof(serv_ip));
  serv_port = get_in_port(serv_info->ai_addr);
  std::cout << "Passive open on " << serv_ip << ":" << serv_port << std::endl;

  return sockfd;
}

int send_err_msg(int sockfd, unsigned short errcode, const struct sockaddr *to, socklen_t tolen) {
  struct tftp_err err;
  char buf[516];
  int buflen;
  err.code = errcode;
  switch (errcode) {
  case 0:
    strcpy(err.msg, "Mode error.");
    break;
  case 1:
    strcpy(err.msg, "File not found.");
    break;
  case 2:
    strcpy(err.msg, "Access violation.");
  }
  buflen = pack_err(&err, buf);
  return sendto(sockfd, buf, buflen, 0, to, tolen);
}

int get_block(const char *filename, int blocknum, char *data) {
  FILE *f;
  f = fopen(filename, "r");
  if (f == NULL) {
    fclose(f);
    return -1;
  }
  // move to the beginning data of blocknum
  fseek(f, (blocknum-1)*512, SEEK_SET);
  return fread(data, 1, 512, f);
}

int send_block(const char *filename, int blocknum, int sockfd, const struct sockaddr *to, socklen_t tolen) {
  char buf[516];
  struct tftp_data block;
  int size_t;
  memset(&block, 0, sizeof(block));
  size_t = get_block(filename, blocknum, block.data);
  size_t = pack_data(&block, buf, size_t);
  return sendto(sockfd, buf, size_t, 0, to, tolen) - 4;
}

int process_rrq(struct addrinfo *serv_info, struct tftp_msg *msg, const struct sockaddr *to, socklen_t tolen) {
  struct tftp_rq rq;
  int sockfd;
  // unpack the request packet
  memset(&rq, 0, sizeof(rq));
  unpack_rq(msg->pld, &rq);
  // create an ephemeral socket using random port number.
  change_in_port(serv_info->ai_addr, 0);
  sockfd = passive_open(serv_info);
  if (sockfd == -1) return -1;
  // Sanity check.
  std::cout << rq.mode << std::endl;// if mode is wrong
  if (access(rq.filename, F_OK) == -1) {  // if file not exists
    std::cout << "File " << rq.filename << " not found." << std::endl;
    send_err_msg(sockfd, 1, to, tolen);
    return -1;
  }
  if (access(rq.filename, R_OK) == -1) {  // if file is not readable.
    std::cout << "File " << rq.filename << " not readable." << std::endl;
    send_err_msg(sockfd, 2, to, tolen);
    return -1;
  }
  std::cout << "File " << rq.filename << " found. Send first ";
  std::cout << send_block(rq.filename, 1, sockfd, to, tolen) << " bytes." << std::endl;
  return sockfd;
}
