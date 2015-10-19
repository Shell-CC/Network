#ifndef TFTP_MSG_H
#define TFTP_MSG_H

// define opcode
#define TFTP_RRQ 1
#define TFTP_WRQ 2
#define TFTP_DATA 3
#define TFTP_ACK 4
#define TFTP_ERROR 5

struct tftp_msg
{
  unsigned short opcode;
  char *pld;
};

struct tftp_rq
{
  char filename[512];
  char mode[16];
};

struct tftp_data
{
  unsigned short block;
  char data[512];
};

struct tftp_ack
{
  unsigned short block;
};

struct tftp_err
{
  unsigned short code;
  char msg[512];
};

void unpack_msg(char *buf, struct tftp_msg *msg);

int unpack_rq(char *pld, struct tftp_rq *rq);

unsigned short unpack_ack(char *pld);

int pack_err(struct tftp_err *err, char *buf);

int pack_data(struct tftp_data *data, char *buf, int datalen);

#endif /* TFTP_MSG_H */