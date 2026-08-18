#pragma once
typedef struct { float kp,ki,kd; float integ; float prev_err; float out_min,out_max; } pid_t;
void pid_init(pid_t* p, float kp, float ki, float kd, float out_min, float out_max);
float pid_update(pid_t* p, float sp, float meas, float dt_s);
void pid_reset(pid_t* p);
