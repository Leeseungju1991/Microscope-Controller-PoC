#pragma once
#include <stdbool.h>
typedef void (*timer_tick_cb_t)(void);
bool timer_drv_init_1ms(timer_tick_cb_t cb);
