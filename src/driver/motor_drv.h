#pragma once
#include <stdint.h>
#include <stdbool.h>
bool motor_drv_init(void);
void motor_drv_set_enable(bool en);
void motor_drv_set_pwm_duty(uint16_t duty_0_to_max);
