#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  FSM_IDLE = 0,
  FSM_RUN  = 1,
  FSM_FAULT= 2,
  FSM_HOLD = 3,
  FSM_CAL  = 4,
} fsm_state_t;

typedef enum {
  FAULT_NONE = 0,
  FAULT_OVERCURRENT,
  FAULT_OVERSPEED,
  FAULT_OVERTEMP,
  FAULT_SENSOR_STALE,
  FAULT_UART_TIMEOUT,
  FAULT_INTERNAL,
} fault_code_t;

typedef struct {
  uint16_t adc_raw[4];

  float current_mA;
  float temp_C;
  float vref_mV;

  int32_t enc_count;
  float position_rev;
  float speed_rpm;

  float target_position_rev;
  float target_speed_rpm;
  float target_current_mA;

  uint16_t pwm_duty;
  bool motor_enable;

  fsm_state_t state;
  fault_code_t fault;

  uint32_t loop_counter;
  uint32_t last_cmd_ms;
  uint32_t last_adc_ms;
} system_ctx_t;
