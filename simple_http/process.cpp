#include "process.h"

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

int send_server_get(struct url_req *req) {
  int sockfd;
  int status;
  // server info
  struct addrinfo hints, *serv_info, *p;
  char serv_ip[INET6_ADDRSTRLEN];
  unsigned short serv_port;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(req->host.c_str(), "http", &hints, &serv_info);
  if (status != 0) {
    std::cout << "ERROR GETTING ADDRESS INFO: " << gai_strerror(status) << std::endl;
    return -1;
  }
  // loop through all the results and connect to the first we can
  bool conn = false;
  for (p = serv_info; p != NULL; p = p->ai_next) {
    // get ip and port
    memset(serv_ip, 0, sizeof(serv_ip));
    inet_ntop(p->ai_family, get_in_addr(p->ai_addr), serv_ip, sizeof(serv_ip));
    serv_port = get_in_port(p->ai_addr);
    // create socket and connect
    if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
      std::cout << "ERROR CREATING SOCKET" << std::endl;
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      std::cout << "Faled to connect to "<< serv_ip << ":" << serv_port <<
                ", trying next..." << std::endl;
      continue;
    }
    std::cout << "Successfully connected to the server " << serv_ip << ":"
              << serv_port << " on socket " << sockfd << std::endl;
    conn = true;
    break;
  }
  if (conn == false) {
    std::cout << "ERROR CONNECTING: faled to connnect to all ip addresses." << std::endl;
    return -1;
  }
  // send GET to server
  string msg;
  msg = string("GET ") + req->resc + string(" HTTP/1.0\r\n")
    + string("Host: ") + req->host + string("\r\n")
    + string("User-Agent: HTMLGET 1.0\r\n\r\n");
  std::cout << "Sending HTTP request...\n" << msg;
  if (send(sockfd, msg.c_str(), msg.length(), 0) < 0) {
    close(sockfd);
    cout << "  ERROR SENDING" << endl;
    return -1;
  }
  std::cout << "Success!" << std::endl;

  return sockfd;
}


int http_recv_write(int sockfd, FILE *fp) {
  // receive buffer
  char buf[1024];
  int rv;
  // recv to buffer and write to file in a loop until done or error
  std::cout << "Receiving...";
  while (1) {
    memset(buf, 0, sizeof(buf));
    rv = recv(sockfd, buf, sizeof(buf), 0);
    if (rv < 0) {
      std::cout << "ERROR RECEIVING: connection closed" << std::endl;
      return -1;
    }
    if (rv == 0) {
      break;
    }
    fwrite(buf, 1, rv, fp);
  }
  std::cout << "Success!" << std::endl;
  return 1;
}

int proxy_send(int sockfd, FILE *fp) {
  // send buffer
  char buf[1024];
  int rv;
  std::cout << "Sending to client...";
  // read from file to buffer and send in a loop until done
  while ((rv = fread(buf, 1, sizeof(buf), fp)) > 0) {
    if (send(sockfd, buf, rv, 0) < 0) {
      return -1;
    }
    memset(buf, 0, sizeof(buf));
  }
  std::cout << "Success!" << std::endl;
  return 1;
}


string parse_header(FILE *fp) {
  char buf[1024];
  string header = "";
  size_t p = -1;
  while (fread(buf, 1, sizeof(buf), fp) > 0) {
    header = header + string(buf);
    p = header.find("\r\n\r\n");
    if (p != string::npos){
      return header.substr(0, p);
    }
    memset(buf, 0, sizeof(buf));
  }
  return "";
}