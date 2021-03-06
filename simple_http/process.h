#ifndef PROCESS_H
#define PROCESS_H

#include <iostream>
#include <cstring>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "http_msg.h"

using namespace std;

void *get_in_addr(struct sockaddr *sa);

unsigned short int get_in_port(struct sockaddr *sa);

int send_server_get(struct url_req *req);

int http_recv_write(int sockfd, FILE *fp);

int proxy_send(int sockfd, FILE *fp);

int send_server_conditional_get(struct url_req *req, time_t t);

string parse_header(FILE *fp);
#endif /* PROCESS_H */