#include "service/encoder.h"
#include "config.h"
static float lowpass(float y, float x, float a){ return a*y + (1.0f-a)*x; }
void encoder_est_init(encoder_est_t* e, int32_t init_count){ e->last_count=init_count; e->speed_rpm=0.0f; e->position_rev=0.0f; }
void encoder_est_update_1khz(encoder_est_t* e, int32_t cnt){
  int32_t d = cnt - e->last_count;
  e->last_count = cnt;
  float rev_per_ms = (float)d / (float)ENCODER_CPR;
  float rpm = rev_per_ms * 60000.0f;
  e->speed_rpm = lowpass(e->speed_rpm, rpm, 0.85f);
  e->position_rev += (float)d / (float)ENCODER_CPR;
}
