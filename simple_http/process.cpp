#include "process.h"
string day[7]={
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

string month[12]={
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

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

int send_server_msg(struct url_req *req, string msg) {
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
  std::cout << "Sending..." ;
  if (send(sockfd, msg.c_str(), msg.length(), 0) < 0) {
    close(sockfd);
    cout << "  ERROR SENDING" << endl;
    return -1;
  }
  std::cout << "Success!" << std::endl;

  return sockfd;
}

int send_server_get(struct url_req *req) {
  string msg;
  msg = string("GET ") + req->resc + string(" HTTP/1.0\r\n")
    + string("Host: ") + req->host + string("\r\n\r\n");
  std::cout << "Forwarding GET request to server..." << std::endl;
  return send_server_msg(req, msg);
}

int send_server_conditional_get(struct url_req *req, time_t t) {
  string msg;
  msg = string("GET ") + req->resc + string(" HTTP/1.0\r\n")
    + string("Host: ") + req->host + string("\r\n");
  struct tm *timenow;
  timenow = gmtime(&t);
  char modified[64];
  sprintf(modified, "%s, %02d %s %4d %02d:%02d:%02d GMT",
          day[timenow->tm_wday].c_str(), timenow->tm_mday,
          month[timenow->tm_mon].c_str(), timenow->tm_year+1900,
          timenow->tm_hour,timenow->tm_min, timenow->tm_sec);
  msg = msg + "If-modified-Since: " + string(modified) + "\r\n";
  msg +=  "\r\n";
  std::cout << "Forwarding conditional GET request to server..." << msg;
  return send_server_msg(req, msg);
}


int http_recv_write(int sockfd, FILE *fp) {
  // receive buffer
  char buf[1024];
  int rv;
  string header = "";
  // recv to buffer and write to file in a loop until done or error
  std::cout << "Receiving...";
  while (1) {
    memset(buf, 0, sizeof(buf));
    rv = recv(sockfd, buf, sizeof(buf), 0);
    header = header + string(buf);
    if (header.find("304 Not Modified") != string::npos) {
      std::cout << "The requested page has not modified." << std::endl;
      return 0;
    }
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