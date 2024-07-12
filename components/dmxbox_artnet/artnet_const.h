#pragma once
#include <stdint.h>

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

enum artnet_node_report_codes {
  artnet_node_report_debug =
      0x0000, // Booted in debug mode (Only used in development)
  artnet_node_report_power_ok = 0x0001,   // Power On Tests successful
  artnet_node_report_power_fail = 0x0002, // Hardware tests failed at Power On
  artnet_node_report_socket_wr1 =
      0x0003, // Last UDP from Node failed due to truncated length,
              // Most likely caused by a collision.
  artnet_node_report_parse_fail =
      0x0004, // Unable to identify last UDP transmission. Check
              // OpCode and packet length.
  artnet_node_report_udp_fail =
      0x0005, // Unable to open Udp Socket in last transmission attempt
  artnet_node_report_sh_name_ok = 0x0006, // Confirms that Port Name programming
                                          // via ArtAddress, was successful.
  artnet_node_report_lo_name_ok = 0x0007, // Confirms that Long Name programming
                                          // via ArtAddress, was successful.
  artnet_node_report_dmx_error = 0x0008, // DMX512 receive errors detected.
  artnet_node_report_dmx_udp_full =
      0x0009, // Ran out of internal DMX transmit buffers.
  artnet_node_report_dmx_rx_full =
      0x000a,                             // Ran out of internal DMX Rx buffers.
  artnet_node_report_switch_err = 0x000b, // Rx Universe switches conflict.
  artnet_node_report_config_err =
      0x000c, // Product configuration does not match firmware.
  artnet_node_report_dmx_short =
      0x000d, // DMX output short detected. See GoodOutput field.
  artnet_node_report_firmware_fail =
      0x000e, // Last attempt to upload new firmware failed.
  artnet_node_report_user_fail =
      0x000f, // User changed switch settings when address locked by
              // remote programming. User changes ignored.
  artnet_node_report_factory_res = 0x0010, // Factory reset has occurred.
};

enum artnet_status_indicator_state {
  artnet_status_indicator_state_unknown = 0, // Indicator state unknown.
  artnet_status_indicator_state_locate_identify =
      1, // Indicators in Locate / Identify Mode.
  artnet_status_indicator_state_mute = 2,   // Indicators in Mute Mode.
  artnet_status_indicator_state_normal = 3, // Indicators in Normal Mode.
};

enum artnet_status_programming_authority {
  artnet_status_programming_authority_unknown =
      0, // Port-Address Programming Authority unknown.
  artnet_status_programming_authority_front_panel =
      1, // All Port-Address set by front panel controls.
  artnet_status_programming_authority_network =
      2, // All or part of Port-Address programmed by network or Web browser.
  artnet_status_programming_authority_unused = 3, // Not used.
};

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
