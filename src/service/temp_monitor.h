#pragma once
#include <stdint.h>
typedef struct { float last_temp_C; } temp_monitor_t;
void temp_monitor_init(temp_monitor_t* tm);
float temp_monitor_calc_C(temp_monitor_t* tm, uint16_t raw_temp, uint16_t raw_vref);
