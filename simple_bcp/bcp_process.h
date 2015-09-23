#include <iostream>
#include <map>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifndef BCP_PROCESS_H
#define BCP_PROCESS_H

int server_recv(int sockfd, char *buf, int len, std::map<int, std::string>* usr_map, int size);

int client_recv(int sockfd, char *buf, int len);

int client_send_join(int sockfd, char *username, int len);

int client_send_msg(int sockfd, char *buf, int len);

int server_send_fwd(int sockfd, char *buf, int buf_len, const char *username, int name_len);

int server_send_online(int sockfd, const char *username, int name_len);

int server_send_offline(int sockfd, const char *username, int name_len);

void *get_in_addr(struct sockaddr *sa);

unsigned short int get_in_port(struct sockaddr *sa);

#endif /* BCP_PROCESS_H */