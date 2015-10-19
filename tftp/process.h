#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "tftp_msg.h"

#ifndef PROCESS_H
#define PROCESS_H

void *get_in_addr(struct sockaddr *sa);

unsigned short int get_in_port(struct sockaddr *sa);

void change_in_port(struct sockaddr *sa, unsigned short port);

int passive_open(struct addrinfo *serv_info);

int process_rrq(struct addrinfo *serv_info, struct tftp_msg *msg, const struct sockaddr *to, socklen_t tolen);

#endif /* PROCESS_H */