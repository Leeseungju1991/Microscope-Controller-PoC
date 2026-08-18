#include "service/cmd_parser.h"
#include "service/crc16.h"
#include "common.h"
#include <string.h>

#define SOF1 0xAA
#define SOF2 0x55

typedef enum { ST_SOF1, ST_SOF2, ST_LEN, ST_BODY, ST_CRC1, ST_CRC2 } st_t;
static st_t st = ST_SOF1;
static uint8_t len = 0;
static uint8_t body[1 + 64];
static uint8_t body_idx = 0;
static uint16_t rx_crc = 0;
static cmd_handler_t g_handler = 0;

void cmd_parser_init(cmd_handler_t handler) { g_handler = handler; st = ST_SOF1; len = 0; body_idx = 0; rx_crc = 0; }
static void reset(void) { st = ST_SOF1; len = 0; body_idx = 0; rx_crc = 0; }

void cmd_parser_feed(uint8_t b) {
  switch (st) {
    case ST_SOF1: if (b == SOF1) st = ST_SOF2; break;
    case ST_SOF2: if (b == SOF2) st = ST_LEN; else st = ST_SOF1; break;
    case ST_LEN:
      len = b;
      if (len < 1 || len > sizeof(body)) { reset(); break; }
      body_idx = 0; st = ST_BODY;
      break;
    case ST_BODY:
      body[body_idx++] = b;
      if (body_idx >= len) st = ST_CRC1;
      break;
    case ST_CRC1: rx_crc = b; st = ST_CRC2; break;
    case ST_CRC2: {
      rx_crc |= ((uint16_t)b << 8);
      uint16_t calc = crc16_ccitt(body, len, 0xFFFF);
      if (calc == rx_crc && g_handler) {
        cmd_msg_t msg;
        msg.id = (cmd_id_t)body[0];
        msg.payload_len = (uint8_t)(len - 1);
        if (msg.payload_len) memcpy(msg.payload, &body[1], msg.payload_len);
        g_handler(&msg);
      } else {
        log_warn("UART drop crc=%04x calc=%04x", rx_crc, calc);
      }
      reset();
      break;
    }
    default: reset(); break;
  }
}

size_t cmd_build_frame(uint8_t cmd, const uint8_t* payload, uint8_t payload_len, uint8_t* out, size_t out_cap) {
  const uint8_t l = (uint8_t)(1 + payload_len);
  const size_t total = 2 + 1 + l + 2;
  if (!out || out_cap < total) return 0;

  size_t i = 0;
  out[i++] = SOF1; out[i++] = SOF2; out[i++] = l;
  out[i++] = cmd;
  for (uint8_t k = 0; k < payload_len; k++) out[i++] = payload[k];
  uint16_t crc = crc16_ccitt(&out[3], l, 0xFFFF);
  out[i++] = (uint8_t)(crc & 0xFF);
  out[i++] = (uint8_t)((crc >> 8) & 0xFF);
  return i;
}
