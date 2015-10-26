#include <iostream>
#include <map>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include "process.h"
#include "http_msg.h"

#define MAX_CLIENTS 20
#define CACHE_SIZE 10

typedef struct cache_node {
  string domain;
  string page;
  string filename;
  time_t last_access;
  time_t expire;
  int has;
}cache_node;

cache_node cache[CACHE_SIZE];
int cache_num = 0;

int cache_contains(struct url_req *req) {
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (req->host.compare(cache[i].domain) == 0
        && req->resc.compare(cache[i].page) == 0) {
      return i;
    }
  }
  return -1;
}

int find_node() {
  // if cache not full, find a empty one.
  int i;
  for (i = 0; i < CACHE_SIZE; i++) {
    if (cache[i].has == 0) {
      return i;
    }
  }
  // if the cache is full, find the least used one
  time_t min = cache[i].last_access;
  i = 0;
  for (int j = 1; j < CACHE_SIZE; j++) {
    if (difftime(cache[i].last_access, min) < 0) {
      min = cache[i].last_access;
      i = j;
    }
  }
  return i;
}

time_t parse_expire_time(string header){
  size_t expirePos = -1;
  expirePos = header.find("\r\nExpires: ");
  string exp="";
  if (expirePos != string::npos) {
    exp = header.substr(expirePos + 11);
  }
  expirePos = exp.find("\r\n");
  string final = "";
  if (expirePos != string::npos) {
    final = exp.substr(0, expirePos);
  } else {
    std::cout << "Cant find expire time in header" << std::endl;
    return 0;
  }
  time_t rawtime;
  struct tm timeinfo;
  std::cout << "File expire time is: " << final << std::endl;
  strptime (final.c_str(),"%a, %d %b %Y %H:%M:%S %Z",&timeinfo);
  rawtime = timegm(&timeinfo);
  return rawtime;
}

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
  FILE *fp;
  // lists
  std::map<int, int> cs_fd;
  std::map<int, std::string> cs_domain;
  std::map<int, std::string> cs_page;
  int ci;
  // timestamp
  time_t now;
  struct tm *timenow;

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
          time(&now);
          timenow = gmtime(&now);
          ci = cache_contains(&req);
          if (ci >= 0) {
            // If in cache, check if it is expired
            std::cout << "Found in the cache, ";
            if (difftime(cache[ci].expire, time(0)) >= 0) {
              // If not expired, get cache and send back
              std::cout << "and not expired." << std::endl;
              fp = fopen(cache[ci].filename.c_str(), "r");
              proxy_send(i, fp);
              fclose(fp);
              // update cache
              cache[ci].last_access = time(0);
              // close connection
              close(i);
              FD_CLR(i, &client_fds);
              FD_CLR(i, &all_fds);
            } else {
              // If expired, send conditional GET to server.
              std::cout << "but expired." << std::endl;
              int send_sockfd = send_server_get(&req);
              if (send_sockfd < 0) {
                close(i);
                FD_CLR(i, &client_fds);
                FD_CLR(i, &all_fds);
                continue;
              }
              cs_fd[send_sockfd] = i;
              cs_domain[send_sockfd] = std::string(req.host);
              cs_page[send_sockfd] = std::string(req.resc);
              FD_SET(send_sockfd, &all_fds);
              fdmax = send_sockfd > fdmax ? send_sockfd : fdmax;
              // Update cache
              cache[ci].has = 0;
            }
          } else {
            // If not in cache, send GET to server
            std::cout << "Not found in cache, sending to server..." << std::endl;
            int send_sockfd = send_server_get(&req);
            if (send_sockfd < 0) {
              close(i);
              FD_CLR(i, &client_fds);
              FD_CLR(i, &all_fds);
              continue;
            }
            cs_fd[send_sockfd] = i;
            cs_domain[send_sockfd] = std::string(req.host);
            cs_page[send_sockfd] = std::string(req.resc);
            FD_SET(send_sockfd, &all_fds);
            fdmax = send_sockfd > fdmax ? send_sockfd : fdmax;
          }
        } else {
          /* handle receiving data from server */
          ci = find_node();
          string filename = "cache" + std::to_string(ci);
          // recive from server
          fp = fopen(filename.c_str(), "w");
          if (http_recv_write(i, fp) < 0) {
            std::cout << "ERROR RECEIVING: try receiving later" << std::endl;
            fclose(fp);
            continue;
          }
          std::cout << "Save to cache: " << filename << std::endl;
          fclose(fp);
          close(i);
          FD_CLR(i, &all_fds);
          // get expire time
          fp = fopen(filename.c_str(), "r");
          string header = parse_header(fp);
          time_t expire = parse_expire_time(header);
          // update cache
          cache[ci].has = 1;
          cache[ci].domain = cs_domain[i];
          cache[ci].page = cs_page[i];
          cache[ci].filename = string(filename);
          cache[ci].expire = expire;
          cache[ci].last_access = time(0);
          // send to client
          int csockfd = cs_fd[i];
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
