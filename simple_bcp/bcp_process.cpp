/*tcp_process.cpp*/
#include <iostream>
#include <map>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "bcp_msg.h"

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

int send_msg(int sockfd, struct bcp_msg *msg) {
  char send_buf[msg->len];
  pack_msg(send_buf, msg);
  return send(sockfd, send_buf, sizeof(send_buf), 0);
}

int server_send_nak(int sockfd, char *reason, int len) {
  struct bcp_msg msg;
  struct bcp_attr attr;
  memset(&msg, 0, sizeof(msg));
  memset(&attr, 0, sizeof(attr));
  msg.vrsn = BCP_VRSN;
  msg.attr = &attr;
  msg.type = BCP_NAK;
  attr.type = BCP_REASON;
  strncpy(attr.pld, reason, len);
  attr.len = len + 4;
  msg.len = attr.len + 4;
  return send_msg(sockfd, &msg);
}

int server_send_ack(int sockfd, std::map<int, std::string>* usr_map) {
  struct bcp_msg msg;
  struct bcp_attr attr;
  struct bcp_attr usr;
  memset(&msg, 0, sizeof(msg));
  memset(&attr, 0, sizeof(attr));
  memset(&usr, 0, sizeof(usr));
  msg.vrsn = BCP_VRSN;
  msg.attr = &attr;
  msg.type = BCP_ACK;
  attr.type = BCP_CC;
  attr.len = sprintf(attr.pld, "%lu", usr_map->size()) + 4;
  attr.next = &usr;
  usr.type = BCP_USERNAME;
  usr.len = 0;
  for (std::map<int, std::string>::iterator it = usr_map->begin(); it != usr_map->end(); ++it) {
    strncpy(&usr.pld[usr.len], it->second.c_str(), it->second.size());
    usr.len += it->second.size();
    usr.pld[usr.len++] = '\t';
  }
  usr.len += 4;
  msg.len = usr.len + attr.len + 4;
  return send_msg(sockfd, &msg);
}

/**
 * Return -1, if connection error or failed to join.
 * Return 0, if succeed to join.
 * Return nbytes>0, if receive message.
 */
int server_recv(int sockfd, char *buf, int len, std::map<int, std::string>* usr_map, int size) {
  char recv_buf[1024];
  int nbytes = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
  if (nbytes <= 0) {
    return -1;
  }
  struct bcp_msg msg;
  memset(&msg, 0, sizeof(msg));
  struct bcp_attr attr;
  memset(&attr, 0, sizeof(attr));
  msg.attr = &attr;
  nbytes = unpack_msg(recv_buf, &msg);
  // check version
  if (msg.vrsn != BCP_VRSN) {
    return 0;
  }
  // check message type
  if (msg.type == BCP_JOIN) {   // JOIN
    // NCK
    if (usr_map->size() >= size) {
      char reason[] = "The chat room is full.";
      server_send_nak(sockfd, reason, sizeof(reason));
      return -1;
    }
    for (std::map<int, std::string>::iterator it = usr_map->begin(); it != usr_map->end(); ++it) {
      if (it->second == attr.pld) {
        char reason[] = "Username existed. ";
        server_send_nak(sockfd, reason, sizeof(reason));
        // std::cout << "--Username existed on socket " << it->first << std::endl;
        return -1;
      }
    }
    // ACK
    (*usr_map)[sockfd] = attr.pld;
    nbytes = server_send_ack(sockfd, usr_map);
    std::cout << "New user " << attr.pld << " joined the chat room " << std::endl;
    return 0;
  } else if (msg.type == BCP_SEND) {   // SEND
    strncpy(buf, attr.pld, attr.len-4);
    std::cout << "--Received " << attr.pld << " from " << usr_map->at(sockfd) << std::endl;
  }
  return attr.len - 4;
}

int client_recv(int sockfd, char *buf, int len) {
  char recv_buf[1024];
  int nbytes = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
  if (nbytes <= 0) {
    return nbytes;
  }
  struct bcp_msg msg;
  memset(&msg, 0, sizeof(msg));
  struct bcp_attr attr;
  memset(&attr, 0, sizeof(attr));
  msg.attr = &attr;
  struct bcp_attr nattr;
  memset(&nattr, 0, sizeof(nattr));
  attr.next = &nattr;
  nbytes = unpack_msg(recv_buf, &msg);
  // check version
  if (msg.vrsn != BCP_VRSN) {
    return 0;
  }
  // check message type
  if (msg.type == BCP_FWD) {   // FWD
    if (attr.type == BCP_MSG) {
      std::cout << attr.pld << std::endl;
    }
    if (nattr.type == BCP_USERNAME) {
      std::cout << "   Sended from " << nattr.pld << std::endl;
    }
  } else if (msg.type == BCP_NAK) { // NAK
    std::cout << "NCK: Can not join the chat room" << std::endl;
    if (attr.type == BCP_REASON) {
      std::cout << attr.pld << std::endl;
    }
    return -1;
  } else if (msg.type == BCP_ACK) { // ACK
    std::cout << "ACK: Welcome to the chat room." << std::endl;
    if (attr.type == BCP_CC) {
      std::cout << "   " << attr.pld << " users are in the chat room: ";
    }
    if (nattr.type == BCP_USERNAME) {
      std::cout << nattr.pld << std::endl;
    }
  } else if (msg.type == BCP_ONLINE) {  // ONLINE
    if (attr.type == BCP_USERNAME) {
      std::cout << "ONLINE: New user " << attr.pld << " joined the chat room." << std::endl;
    }
  } else if (msg.type == BCP_OFFLINE) {  // OFFLINE
    if (attr.type == BCP_USERNAME) {
      std::cout << "OFFLINE: User " << attr.pld << " left the chat room." << std::endl;
    }
  }
  return msg.len;
}

int client_send_join(int sockfd, char *username, int len) {
  struct bcp_msg msg;
  struct bcp_attr usr;
  memset(&msg, 0, sizeof(msg));
  memset(&usr, 0, sizeof(usr));
  // set attribute as username
  usr.type = BCP_USERNAME;
  strncpy(usr.pld, username, len);
  usr.len = len + 4;
  // set message as join
  msg.vrsn = BCP_VRSN;
  msg.type = BCP_JOIN;
  msg.attr = &usr;
  msg.len = usr.len + 4;
  // pack and send
  return send_msg(sockfd, &msg);
}

int client_send_msg(int sockfd, char *buf, int len) {
  struct bcp_msg msg;
  struct bcp_attr attr;
  memset(&msg, 0, sizeof(msg));
  memset(&attr, 0, sizeof(attr));
  // set attribute as message
  attr.type = BCP_MSG;
  strncpy(attr.pld, buf, len);
  attr.len = len + 4;
  // set message as join
  msg.vrsn = BCP_VRSN;
  msg.type = BCP_SEND;
  msg.attr = &attr;
  msg.len = attr.len + 4;
  // pack and send
  return send_msg(sockfd, &msg);
}

int server_send_fwd(int sockfd, char *buf, int buf_len, const char *username, int name_len) {
  struct bcp_msg msg;
  struct bcp_attr attr;
  struct bcp_attr usr;
  memset(&msg, 0, sizeof(msg));
  memset(&attr, 0, sizeof(attr));
  memset(&usr, 0, sizeof(usr));
  // set attribute as message
  attr.type = BCP_MSG;
  strncpy(attr.pld, buf, buf_len);
  attr.len = buf_len + 4;
  attr.next = &usr;
  // set attribute as user
  usr.type = BCP_USERNAME;
  strncpy(usr.pld, username, name_len);
  usr.len = name_len + 4;
  // set message as join
  msg.vrsn = BCP_VRSN;
  msg.type = BCP_FWD;
  msg.attr = &attr;
  msg.len = attr.len + usr.len + 4;
  // pack and send
  return send_msg(sockfd, &msg);
}

int server_send_online(int sockfd, const char *username, int name_len) {
  struct bcp_msg msg;
  struct bcp_attr usr;
  memset(&msg, 0, sizeof(msg));
  memset(&usr, 0, sizeof(usr));
  // set attribute as USER
  usr.type = BCP_USERNAME;
  strncpy(usr.pld, username, name_len);
  usr.len = name_len + 4;
  // set message as ONLINE
  msg.vrsn = BCP_VRSN;
  msg.type = BCP_ONLINE;
  msg.attr = &usr;
  msg.len = usr.len + 4;
  //
  return send_msg(sockfd, &msg);
}
int server_send_offline(int sockfd, const char *username, int name_len) {
  struct bcp_msg msg;
  struct bcp_attr usr;
  memset(&msg, 0, sizeof(msg));
  memset(&usr, 0, sizeof(usr));
  // set attribute as USER
  usr.type = BCP_USERNAME;
  strncpy(usr.pld, username, name_len);
  usr.len = name_len + 4;
  // set message as ONLINE
  msg.vrsn = BCP_VRSN;
  msg.type = BCP_OFFLINE;
  msg.attr = &usr;
  msg.len = usr.len + 4;
  //
  return send_msg(sockfd, &msg);
}