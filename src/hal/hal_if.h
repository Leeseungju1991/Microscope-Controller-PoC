#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
bool hal_init(void);
bool hal_adc_start(void);
void hal_adc_stop(void);
bool hal_uart_send(const uint8_t* data, size_t len);
void hal_motor_enable(bool en);
void hal_motor_set_duty(uint16_t duty);
int32_t hal_encoder_get_count(void);
bool hal_camera_snapshot(uint16_t delay_us, uint16_t width_us);
