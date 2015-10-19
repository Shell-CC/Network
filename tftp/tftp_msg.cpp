#include <string>
#include "tftp_msg.h"

void unpack_msg(char *buf, struct tftp_msg *msg) {
  msg->opcode = (*buf++)<<8;
  msg->opcode = msg->opcode | ((*buf++) & 0xFF);
  msg->pld = buf;
}

int unpack_rq(char *pld, struct tftp_rq *rq) {
  int i, j;
  for (i = 0; *pld != '\0'; i++) {
    rq->filename[i] = *(pld++);
  }
  pld++;
  for (j = 0; *pld != '\0'; j++) {
    rq->mode[j] = *(pld++);
  }
  return i+j;
}

unsigned short unpack_ack(char *pld) {
  unsigned short blocknum;
  blocknum = (*pld++)<<8;
  blocknum = blocknum | ((*pld++) & 0xFF);
  return blocknum;
}

int pack_err(struct tftp_err *err, char *buf) {
  unsigned short opcode = 5;
  *buf++ = opcode >> 8; *buf++ = opcode;
  *buf++ = err->code >> 8; *buf++ = err->code;
  int len = sizeof(err->msg);
  strncpy(buf, err->msg, len);
  return len+4;
}

int pack_data(struct tftp_data *data, char *buf, int datalen) {
  unsigned short opcode = 3;
  *buf++ = opcode >> 8; *buf++ = opcode;
  *buf++ = data->block >> 8; *buf++ = data->block;
  strncpy(buf, data->data, datalen);
  return datalen + 4;
}
