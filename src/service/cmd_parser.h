#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum {
  CMD_START = 0x01,
  CMD_STOP  = 0x02,
  CMD_SET_TARGET_SPEED = 0x03,     // int16 rpm
  CMD_GET_STATUS = 0x04,
  CMD_CLEAR_FAULT = 0x05,

  CMD_SET_PID_DUAL = 0x06,         // 12 bytes: pos(kp,ki,kd) + spd(kp,ki,kd) as Q8.8
  CMD_SET_TARGET_POS = 0x07,       // int32 position_mrev (milli-rev)
  CMD_SET_MOTION_LIMITS = 0x08,    // int16 max_rpm, int16 accel_rpm_s
  CMD_CAMERA_SNAPSHOT = 0x09,      // uint16 delay_us, uint16 width_us
  CMD_SET_TARGET_CURRENT = 0x0A,   // uint16 mA
  CMD_SET_PID_CURRENT = 0x0B,      // 4 bytes: kp,ki as Q8.8
} cmd_id_t;

typedef struct {
  cmd_id_t id;
  uint8_t payload[64];
  uint8_t payload_len;
} cmd_msg_t;

typedef void (*cmd_handler_t)(const cmd_msg_t* msg);

void cmd_parser_init(cmd_handler_t handler);
void cmd_parser_feed(uint8_t byte);
size_t cmd_build_frame(uint8_t cmd, const uint8_t* payload, uint8_t payload_len, uint8_t* out, size_t out_cap);
