#include <iostream>
#include <sys/socket.h>
#include "bcp_msg.h"

int pack_msg(char *buf, struct bcp_msg *msg) {
  unsigned short vt;
  struct bcp_attr *attr;
  // pack header
  vt = ((msg->vrsn) << 7) | (msg->type);
  *buf++ = vt>>8; *buf++ = vt;
  *buf++ = (msg->len)>>8; *buf++ = msg->len;
  // pack attributes
  for (attr = msg->attr; attr != NULL; attr = attr->next) {
    // pack attribute header
    *buf++ = (attr->type)>>8; *buf++ = attr->type;
    *buf++ = (attr->len)>>8; *buf++ = attr->len;
    // pack attribute payload
    strncpy(buf, attr->pld, attr->len-4);
    // std::cout << attr->len << " " << msg->len << std::endl;
    buf = buf + (attr->len - 4);
  }
  return msg->len;
}

int unpack_msg(char *buf, struct bcp_msg *msg) {
  unsigned short vt;
  struct bcp_attr *attr;
  int i = 0;
  // unpack header
  vt = (*buf++)<<8; vt = vt | ((*buf++) & 0xFF);
  msg->vrsn = (vt & 0xFF80) >> 7;
  msg->type = vt & 0x7F;
  msg->len = (*buf++)<<8; msg->len = msg->len | (*(buf++) & 0xFF);
  i = i+4;
  for (attr = msg->attr; attr != NULL; attr = attr->next) {
    // unpack attribute header
    attr->type = (*buf++)<<8; attr->type = attr->type | (*(buf++) & 0xFF);
    attr->len = (*buf++)<<8; attr->len = attr->len | (*(buf++) & 0xFF);
    strncpy(attr->pld, buf, attr->len-4);
    // std::cout << i << " " << attr->len << " " << msg->len << std::endl;
    i = i + attr->len;
    if (i >= msg->len) {
      break;
    }
    buf = buf + (attr->len - 4);
  }
  return msg->len;
}
