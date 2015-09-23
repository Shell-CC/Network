
#define BCP_VRSN 3
// define SBCP Header types
#define BCP_JOIN 2
#define BCP_SEND 4
#define BCP_FWD 3
#define BCP_ACK 7
#define BCP_NAK 5
#define BCP_ONLINE 8
#define BCP_OFFLINE 6
#define BCP_IDLE 9
// define Attribute types
#define BCP_USERNAME 2
#define BCP_MSG 4
#define BCP_REASON 1
#define BCP_CC 3

struct bcp_msg
{
  unsigned short vrsn;
  unsigned short type;
  unsigned short len;
  struct bcp_attr *attr;
};

struct bcp_attr
{
  unsigned short type;
  unsigned short len;
  char pld[512];
  struct bcp_attr *next;
};

int pack_msg(char *buf, struct bcp_msg *msg);

int unpack_msg(char *buf, struct bcp_msg *msg);