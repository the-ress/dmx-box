#define ARTNET_PORT 6454
#define PACKET_ID "Art-Net\0"
#define ARTNET_VERSION 14
#define MAX_PACKET_SIZE 530

#define OP_POLL 0x2000
#define OP_POLL_REPLY 0x2100
#define OP_DMX 0x5000

#define PORT_TYPE_OUTPUT 0x80
#define OEM_UNKNOWN                                                            \
  0xff // https://github.com/tobiasebsen/ArtNode/blob/master/src/Art-NetOemCodes.h
#define ST_NODE 0 // A DMX to / from Art-Net device

struct artnet_reply_s {
  uint8_t id[8];
  uint16_t opCode;
  uint8_t ip[4];
  uint16_t port;
  uint8_t verH;
  uint8_t ver;
  uint8_t subH;
  uint8_t sub;
  uint8_t oemH;
  uint8_t oem;
  uint8_t ubea;
  uint8_t status;
  uint8_t etsaman[2];
  uint8_t shortname[18];
  uint8_t longname[64];
  uint8_t nodereport[64];
  uint8_t numbportsH;
  uint8_t numbports;
  uint8_t porttypes[4]; // max of 4 ports per node
  uint8_t goodinput[4];
  uint8_t goodoutput[4];
  uint8_t swin[4];
  uint8_t swout[4];
  uint8_t swvideo;
  uint8_t swmacro;
  uint8_t swremote;
  uint8_t sp1;
  uint8_t sp2;
  uint8_t sp3;
  uint8_t style;
  uint8_t mac[6];
  uint8_t bindip[4];
  uint8_t bindindex;
  uint8_t status2;
  uint8_t filler[26];
} __attribute__((packed));
