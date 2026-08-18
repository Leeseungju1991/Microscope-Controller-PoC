#pragma once
#include <stdint.h>
#include <stdbool.h>
bool camera_trigger_drv_init(void);
bool camera_trigger_fire(uint16_t delay_us, uint16_t width_us);
