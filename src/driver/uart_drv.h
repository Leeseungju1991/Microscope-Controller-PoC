#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef void (*uart_rx_cb_t)(uint8_t b);
bool uart_drv_init(uint32_t baud, uart_rx_cb_t rx_cb);
bool uart_drv_tx(const uint8_t* data, size_t len);
void uart_drv_feed_rx_byte(uint8_t b);
