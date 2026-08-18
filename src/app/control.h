#pragma once
#include "types.h"
#include "app/pid.h"
#include "app/motion_profile.h"
#include "service/encoder.h"

typedef struct {
  pid_t pos_pid;
  pid_t spd_pid;
  pid_t cur_pi;     // PI (kd=0)
  motion_profile_t motion;
  encoder_est_t enc;
} control_t;

void control_init(control_t* c, system_ctx_t* ctx);
void control_step_1khz(control_t* c, system_ctx_t* ctx, float dt_s);
