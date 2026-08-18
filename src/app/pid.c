#include "app/pid.h"
static float clampf(float v,float lo,float hi){ if(v<lo)return lo; if(v>hi)return hi; return v; }
void pid_init(pid_t* p,float kp,float ki,float kd,float out_min,float out_max){
  p->kp=kp;p->ki=ki;p->kd=kd;p->integ=0.0f;p->prev_err=0.0f;p->out_min=out_min;p->out_max=out_max;
}
void pid_reset(pid_t* p){ p->integ=0.0f;p->prev_err=0.0f; }
float pid_update(pid_t* p,float sp,float meas,float dt_s){
  float err = sp - meas;
  p->integ += err * dt_s;
  float deriv = (dt_s>0.0f)?(err - p->prev_err)/dt_s:0.0f;
  p->prev_err = err;
  float out = p->kp*err + p->ki*p->integ + p->kd*deriv;
  out = clampf(out, p->out_min, p->out_max);
  if(out==p->out_min || out==p->out_max) p->integ *= 0.98f;
  return out;
}
