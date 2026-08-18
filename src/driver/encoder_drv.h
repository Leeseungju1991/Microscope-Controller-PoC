#pragma once
#include <stdint.h>
#include <stdbool.h>
bool encoder_drv_init(void);
bool encoder_drv_start(void);
int32_t encoder_drv_get_count(void);
