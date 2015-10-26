#ifndef HTTP_MSG_H
#define HTTP_MSG_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

struct url_req
{
  string host;
  string resc;
};

void parse_url(string url, struct url_req *req);

string pack_header_get(string url);

int unpack_header_get(string msg, struct url_req *req);

#endif /* HTTP_MSG_H */