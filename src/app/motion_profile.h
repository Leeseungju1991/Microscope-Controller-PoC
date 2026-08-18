#pragma once
typedef struct { float max_speed_rpm; float max_accel_rpm_s; float spd_rpm; } motion_profile_t;
void motion_profile_init(motion_profile_t* m, float max_speed_rpm, float max_accel_rpm_s);
float motion_profile_step_speed(motion_profile_t* m, float target_speed_rpm, float dt_s);
