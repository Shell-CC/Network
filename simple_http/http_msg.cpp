#include "http_msg.h"

void parse_url(string url, struct url_req *req)
{
  size_t isHttpFound = -1;
  isHttpFound = url.find("http://");
  if (isHttpFound != string::npos) {
    url = url.substr(isHttpFound + 7);
  }
  size_t isHttpsFound = -1;
  isHttpsFound = url.find("https://");
  if (isHttpsFound != string::npos) {
    url = url.substr(isHttpsFound + 8);
  }
  size_t host_end = url.find("/");
  req->host = url.substr(0, host_end);
  if(host_end == string::npos) {
    req->resc = string("/");
  } else {
    req->resc = url.substr(host_end);
  }
}


string pack_header_get(string url) {
  struct url_req req;
  string header;
  parse_url(url, &req);
  header = string("GET ") + req.resc + string(" HTTP/1.0\r\n")
    + string("Host: ") + req.host + string("\r\n")
    + string("User-Agent: HTMLGET 1.0\r\n\r\n");
  return header;
}


int unpack_header_get(string msg, struct url_req *req) {
  size_t p, q;
  // std::cout << msg << std::endl;
  p = msg.find("GET ");
  if (p == string::npos) {
    return -1;
  } else {
    p = p+4;
  }
  q = msg.find(" HTTP/1.0\r\n", p);
  if (q == string::npos) {
    return -1;
  } else {
    req->resc = msg.substr(p, q-p);
  }
  p = msg.find("Host: ");
  if (p == string::npos) {
    return -1;
  } else {
    p = p+6;
  }
  q = msg.find("\r\n", p);
  if (q == string::npos) {
    return -1;
  } else {
    req->host = msg.substr(p, q-p);
  }
  return 0;
}