#ifndef BCP_PROCESS_H
#define BCP_PROCESS_H

int process_send(int sockfd, const void *msg, int len, int flags);

int process_recv(int sockfd, void *buf, int len, int flags);

void *get_in_addr(struct sockaddr *sa);

unsigned short int get_in_port(struct sockaddr *sa);

#endif /* BCP_PROCESS_H */