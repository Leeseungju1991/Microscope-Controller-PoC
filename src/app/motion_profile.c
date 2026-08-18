#include "app/motion_profile.h"
static float clampf(float v,float lo,float hi){ if(v<lo)return lo; if(v>hi)return hi; return v; }
void motion_profile_init(motion_profile_t* m,float max_speed_rpm,float max_accel_rpm_s){
  m->max_speed_rpm=max_speed_rpm; m->max_accel_rpm_s=max_accel_rpm_s; m->spd_rpm=0.0f;
}
float motion_profile_step_speed(motion_profile_t* m,float target_speed_rpm,float dt_s){
  target_speed_rpm = clampf(target_speed_rpm, -m->max_speed_rpm, m->max_speed_rpm);
  float delta = target_speed_rpm - m->spd_rpm;
  float max_step = m->max_accel_rpm_s * dt_s;
  if(delta> max_step) delta = max_step;
  if(delta<-max_step) delta = -max_step;
  m->spd_rpm += delta;
  return m->spd_rpm;
}
