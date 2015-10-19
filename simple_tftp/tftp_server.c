#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

// define opcode
#define RRQ 0x01
#define WRQ 0x02
#define DATA 0x03
#define ACK 0x04
#define ERROR 0x05

#define BUFFERSIZE 516
#define FILENAMESIZE 256
#define MODESIZE 12
#define BLOCKLEN 512
#define TRYROUND 50
#define TIMEUSEC 100000


//store messages for all the client currently 'connected' with server.
typedef struct clientlist
{
    struct clientlist *prior;
    pthread_t threadid;
    char filename[FILENAMESIZE];
    char mode[MODESIZE];
    struct sockaddr_in addressinfo;
    struct clientlist *next;
}clientlist;

int add_header(char *dest, char *data);
void blocknumadd(unsigned char *high, unsigned char *low);

clientlist* creatclientlist()
{
  clientlist *head;
  head = (clientlist*)malloc(sizeof(clientlist));
  head->next=NULL;
  return head;
}

clientlist* insert(struct sockaddr_in *addr, char *filename, clientlist *head)
{
  clientlist *l = NULL;
  clientlist *p = NULL;
  l = head;
  while (l->next!=NULL) {
    l = l->next;
  }
  p = (clientlist*)malloc(sizeof(clientlist));
  memset(p,0, sizeof(clientlist));
  p->addressinfo = *addr;
  strcpy(p->filename, filename);
  p->prior=l;
  l->next=p;
  return p;
}

void delete(clientlist *listnode)
{
  listnode->prior->next=listnode->next;
  free(listnode);
}

// get address info
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

//socket initializaton.
int passive_open(const char *node, const char *service) {
  struct addrinfo hints, *serv_info;
  char serv_ip[INET6_ADDRSTRLEN];
  unsigned short int serv_port;
  int sockfd;

  // get address info of the server
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  if (getaddrinfo(node, service, &hints, &serv_info) != 0) {
    printf("ERROR GETTING ADDRESS INFO\n");
    exit(1);
  }
  // Create a socket for the server
  sockfd = socket(serv_info->ai_family, serv_info->ai_socktype, serv_info->ai_protocol);
  if (sockfd == -1) {
    perror("ERROR CREATING SOCKET: ");
    exit(1);
  }
  // Passive open: bind and listen.
  if (bind(sockfd, serv_info->ai_addr, serv_info->ai_addrlen) == -1) {
    perror("ERROR BINDING: ");
    exit(1);
  }
  // get the real ip and port number which are passive open.
  getsockname(sockfd, serv_info->ai_addr, &serv_info->ai_addrlen);
  inet_ntop(serv_info->ai_family, get_in_addr(serv_info->ai_addr), serv_ip, sizeof(serv_ip));
  serv_port = get_in_port(serv_info->ai_addr);
  printf("Passive open on %s:%d\n", serv_ip, serv_port);

  return sockfd;
}

//this is the function for each thread.
int tftprecv(clientlist *client)
{
  int newsock;
  int readlength = -1;
  int length, recvlen;
  char filename[FILENAMESIZE]={0};
  unsigned char bufrecv[128]={0};
  char mode[MODESIZE]={0};
  char buftosend[BUFFERSIZE]={0};
  char fileblock[BUFFERSIZE]={0};
  struct sockaddr_in clientaddr;
  struct sockaddr_in recvaddr;
  struct timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = TIMEUSEC;

  FILE *fp;
  clientaddr = client->addressinfo;

  strcpy(filename, client->filename);
  strcpy(mode, client->mode);
  printf(" file: %s, with mode: %s\n", filename, mode);

  //create new socket for "connection"(send data and receive ACKs)
  if ((newsock = socket(AF_INET, SOCK_DGRAM, 0))<0) {
    printf("  ERROR: creating socket failed.");
    delete(client);
    return -1;
  }

  // file not found error
  fp = fopen(filename, "r");
  if (fp==NULL) {
    printf("  ERROR: File not found\n");
    length = sprintf(buftosend, "%c%c%c%cFile not found%c\n",0x00,0x05,0x00,0x01,0x00);
    sendto(newsock, buftosend, length, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
    fclose(fp);
    close(newsock);
    return -1;
  }

  // unknow mode error
  if ((strcmp(mode, "octet") != 0) && (strcmp(mode, "netascii") != 0)) {
    printf("  ERROR: Unknown mode.\n");
    length = sprintf(buftosend, "%c%c%c%cUnknown mode%c\n",0x00,0x05,0x00,0x00,0x00);
    sendto(newsock, buftosend, length, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
    fclose(fp);
    close(newsock);
    return -1;
  }

  printf("  File found. Sending...\n");
  unsigned char blocknumh,blocknuml;
  blocknumh=0x00;
  blocknuml=0x00;
  int recvsignal = 0;
  socklen_t g = sizeof(clientaddr);
  clock_t time,waittime;
  fd_set fds;
  //this while loop is for sending data and receiving acks
  while (1) {
    //printf("%d\n",count++);
    memset(buftosend, 0, BUFFERSIZE);
    readlength = fread(fileblock, 1, BLOCKLEN, fp);
    //printf("%d",readlength);
    blocknumadd(&blocknumh, &blocknuml);
    sprintf(buftosend, "%c%c%c%c", 0x00, 0x03, blocknumh, blocknuml);
    memcpy(buftosend+4, fileblock, readlength);

    //this for loop is for resending data package when lost acks.
    for (int i=0; i<TRYROUND; i++){
      recvsignal = 0;
      tv.tv_usec = TIMEUSEC;
      sendto(newsock, buftosend, readlength+4, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
      time = clock();
      //this while loop is for receiving acks. Using select with timeout 100ms for each socket.
      //Calling clock() when data received in order to get the total time after last data package was sent. And call select again with timeout 100ms-timeused when receiving a wrong ACK.
      while (1) {
        FD_ZERO(&fds);
        FD_SET(newsock,&fds);
        if(select(newsock+1, &fds, NULL, NULL, &tv)>0){
          memset(bufrecv, 0, 128);
          recvlen = recvfrom(newsock, bufrecv, BUFFERSIZE, 0, (struct sockaddr *)&recvaddr, &g);
          if (recvlen==4) {
            if ((*bufrecv==0x00)&&(*(bufrecv+1)==0x04)&&(*(bufrecv+2)==blocknumh)&&(*(bufrecv+3)==blocknuml)) {
              recvsignal = 1;
              break;
            }
            else;
          }
          else;
          waittime = clock()-time;
          tv.tv_usec=TIMEUSEC-waittime*1000;
          if (tv.tv_usec<=0) {
            //printf("select timeout case 1\n");
            break;
          }
        }
        else{
          //printf("select timeout\n");
          break;
        }
      }
      if (recvsignal) break;
    }
    if (!recvsignal) {
      printf("  Timeout! No response from client %s:%d\n",inet_ntoa(clientaddr.sin_addr),clientaddr.sin_port);
      break;
    }
    else{
      memset(&recvaddr,0, sizeof(recvaddr));
      memset(fileblock,0, BLOCKLEN);
    }
    // detect whether the file is completely transfered.
    if (readlength<512) {
      printf("  Successed.\n");
      break;
    }
  }
  // release all resources after send complete/
  close(newsock);
  delete(client);
  fclose(fp);
  return 0;
}

void blocknumadd(unsigned char *high, unsigned char *low)
{
    if ((*low)!=0xFF) {
      (*low)++;
    } else {
      (*low)=0x00;
      (*high)++;
    }
}

int main(int argc, const char *argv[]) {

  if (argc != 3) {
    printf("Usage: ./server server_ip server_port\n");
    return -1;
  }

  // server info
  int sockfd;
  int len;

  // data structures
  clientlist *head;
  clientlist *client;
  char filename[FILENAMESIZE];
  char buf[BUFFERSIZE];
  char *bufpos;

  // new client info
  socklen_t clientlen;
  struct sockaddr_in clientaddr;

  // create a main socket and listen
  sockfd = passive_open(argv[1], argv[2]);
  printf("Server is listening on socket %d\n", sockfd);

  head = creatclientlist();

  // This main thread is for handling new connections.
  while (1) {
    memset(buf,0, BUFFERSIZE);
    memset(&clientaddr, 0,sizeof(clientaddr));
    clientlen = sizeof(clientaddr);
    len = recvfrom(sockfd, buf, BUFFERSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
    if (len < 0) {
      printf("ERROR RECEIVING.\n");
      return -1;
    } else if (len == 0) {
      printf("ERROR: Unexpected data length.\n");
      continue;
    }
    bufpos = buf;
    if (*bufpos != 0) {
      printf("ERROR: Unexpected data received.\n");
      return -1;
    }
    bufpos++;
    switch (*bufpos) {
    case WRQ:
      printf("WRQ: Not yet implimented.\n");
      break;
    case RRQ:
      memset(filename,0, FILENAMESIZE);
      bufpos++;
      printf("RRQ from %s:%d, ",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
      strcpy(filename, bufpos);
      bufpos = bufpos + strlen(filename) + 1;
      client = insert(&clientaddr, filename, head);
      strcpy(client->mode, bufpos);
      //creat new thread to handle with a accepted requect.
      pthread_create(&(client->threadid), NULL, (void *)(&tftprecv), (void *)client);
      break;
    default:
      printf("ERROR: Unexpected option code.\n");
      break;
    }
  }
  return 0;
}
