#include "app/control.h"
#include "hal/hal_if.h"
#include "config.h"

static float clampf(float v,float lo,float hi){ if(v<lo)return lo; if(v>hi)return hi; return v; }

void control_init(control_t* c, system_ctx_t* ctx){
  pid_init(&c->pos_pid, 2.0f, 0.0f, 0.05f, -MAX_SPEED_RPM, MAX_SPEED_RPM);
  pid_init(&c->spd_pid, 0.6f, 0.08f, 0.0f, CURRENT_SP_MIN_mA, CURRENT_SP_MAX_mA);
  pid_init(&c->cur_pi,  0.002f, 0.15f, 0.0f, PID_OUT_MIN, PID_OUT_MAX);
  motion_profile_init(&c->motion, MAX_SPEED_RPM, MAX_ACCEL_RPM_S);
  int32_t cnt = hal_encoder_get_count();
  encoder_est_init(&c->enc, cnt);

  ctx->target_position_rev = 0.0f;
  ctx->target_speed_rpm = 0.0f;
  ctx->target_current_mA = 0.0f;
}

void control_step_1khz(control_t* c, system_ctx_t* ctx, float dt_s){
  int32_t cnt = hal_encoder_get_count();
  encoder_est_update_1khz(&c->enc, cnt);

  ctx->enc_count = cnt;
  ctx->position_rev = c->enc.position_rev;
  ctx->speed_rpm = c->enc.speed_rpm;

  float speed_sp = ctx->target_speed_rpm;
  float pos_err = ctx->target_position_rev - ctx->position_rev;
  if (pos_err > 0.0001f || pos_err < -0.0001f) {
    speed_sp = pid_update(&c->pos_pid, ctx->target_position_rev, ctx->position_rev, dt_s);
  }

  speed_sp = motion_profile_step_speed(&c->motion, speed_sp, dt_s);
  ctx->target_speed_rpm = speed_sp;

  float cur_sp = pid_update(&c->spd_pid, ctx->target_speed_rpm, ctx->speed_rpm, dt_s);
  cur_sp = clampf(cur_sp, CURRENT_SP_MIN_mA, CURRENT_SP_MAX_mA);
  ctx->target_current_mA = cur_sp;

  float duty_norm = pid_update(&c->cur_pi, ctx->target_current_mA, ctx->current_mA, dt_s);
  duty_norm = clampf(duty_norm, PID_OUT_MIN, PID_OUT_MAX);

  ctx->pwm_duty = (uint16_t)(duty_norm * (float)MOTOR_PWM_MAX_DUTY);
  if(ctx->pwm_duty > MOTOR_PWM_MAX_DUTY) ctx->pwm_duty = MOTOR_PWM_MAX_DUTY;
}
