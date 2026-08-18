#pragma once
#include <stdint.h>
typedef struct { int32_t last_count; float speed_rpm; float position_rev; } encoder_est_t;
void encoder_est_init(encoder_est_t* e, int32_t init_count);
void encoder_est_update_1khz(encoder_est_t* e, int32_t count_now);
