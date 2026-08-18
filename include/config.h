#pragma once

#define SCHED_TICK_MS                  (1u)
#define TASK_CTRL_1KHZ_MS              (1u)
#define TASK_SLOW_100HZ_MS             (10u)
#define TASK_TELEM_10HZ_MS             (100u)

#define ADC_NUM_CHANNELS               (4u)
#define ADC_DMA_FRAMES                 (64u)

#define ADC_CH_CURRENT_IDX             (0u)
#define ADC_CH_RESERVED_IDX            (1u)
#define ADC_CH_TEMP_IDX                (2u)
#define ADC_CH_VREF_IDX                (3u)

#define MOTOR_PWM_MAX_DUTY             (1000u)
#define MOTOR_PWM_MIN_DUTY             (0u)

#define ENCODER_CPR                    (2048)
#define ENCODER_SAMPLE_MS              (1u)

#define MAX_SPEED_RPM                  (3000.0f)
#define MAX_ACCEL_RPM_S                (8000.0f)

#define PROT_OVERCURRENT_mA            (2500)
#define PROT_OVERSPEED_RPM             (3500)
#define PROT_OVERTEMP_C                (85.0f)
#define PROT_SENSOR_STALE_MS           (50u)
#define PROT_UART_TIMEOUT_MS           (500u)

#define PID_OUT_MIN                    (0.0f)
#define PID_OUT_MAX                    (1.0f)

#define CURRENT_SP_MIN_mA              (0.0f)
#define CURRENT_SP_MAX_mA              (2500.0f)
