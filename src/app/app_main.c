#include "app/app_main.h"
#include "types.h"
#include "common.h"
#include "config.h"

#include "app/scheduler.h"
#include "app/fsm.h"
#include "app/control.h"

#include "hal/hal_if.h"

#include "service/cmd_parser.h"
#include "service/protection.h"
#include "service/sensor.h"
#include "service/telemetry.h"
#include "service/camera.h"

#include "driver/adc_drv.h"
#include "driver/uart_drv.h"
#include "driver/timer_drv.h"

#include <string.h>

static system_ctx_t g_ctx;
static control_t g_ctrl;

static uint16_t g_adc_dma_words[ADC_DMA_FRAMES * ADC_NUM_CHANNELS];

static void adc_half_cb(void){ sensor_on_adc_dma_block(&g_ctx, &g_adc_dma_words[0], (ADC_DMA_FRAMES/2)); }
static void adc_full_cb(void){ sensor_on_adc_dma_block(&g_ctx, &g_adc_dma_words[(ADC_DMA_FRAMES/2)*ADC_NUM_CHANNELS], (ADC_DMA_FRAMES/2)); }

static void on_uart_rx(uint8_t b){ cmd_parser_feed(b); }

static float q88_to_f(int16_t q){ return (float)q / 256.0f; }

static void handle_cmd(const cmd_msg_t* msg){
  g_ctx.last_cmd_ms = platform_millis();

  switch(msg->id){
    case CMD_START: fsm_request_start(&g_ctx); break;
    case CMD_STOP:  fsm_request_stop(&g_ctx); break;

    case CMD_SET_TARGET_SPEED:
      if(msg->payload_len>=2){
        int16_t rpm=(int16_t)((msg->payload[1]<<8)|msg->payload[0]);
        g_ctx.target_speed_rpm=(float)rpm;
      }
      break;

    case CMD_SET_TARGET_POS:
      if(msg->payload_len>=4){
        int32_t mrev=(int32_t)((msg->payload[3]<<24)|(msg->payload[2]<<16)|(msg->payload[1]<<8)|msg->payload[0]);
        g_ctx.target_position_rev=(float)mrev/1000.0f;
        g_ctx.state = FSM_HOLD;
      }
      break;

    case CMD_SET_TARGET_CURRENT:
      if(msg->payload_len>=2){
        uint16_t ma=(uint16_t)((msg->payload[1]<<8)|msg->payload[0]);
        g_ctx.target_current_mA=(float)ma;
      }
      break;

    case CMD_SET_MOTION_LIMITS:
      if(msg->payload_len>=4){
        int16_t maxrpm=(int16_t)((msg->payload[1]<<8)|msg->payload[0]);
        int16_t accel =(int16_t)((msg->payload[3]<<8)|msg->payload[2]);
        g_ctrl.motion.max_speed_rpm=(float)maxrpm;
        g_ctrl.motion.max_accel_rpm_s=(float)accel;
      }
      break;

    case CMD_SET_PID_DUAL:
      if(msg->payload_len>=12){
        int16_t pkp=(int16_t)((msg->payload[1]<<8)|msg->payload[0]);
        int16_t pki=(int16_t)((msg->payload[3]<<8)|msg->payload[2]);
        int16_t pkd=(int16_t)((msg->payload[5]<<8)|msg->payload[4]);
        int16_t skp=(int16_t)((msg->payload[7]<<8)|msg->payload[6]);
        int16_t ski=(int16_t)((msg->payload[9]<<8)|msg->payload[8]);
        int16_t skd=(int16_t)((msg->payload[11]<<8)|msg->payload[10]);
        g_ctrl.pos_pid.kp=q88_to_f(pkp); g_ctrl.pos_pid.ki=q88_to_f(pki); g_ctrl.pos_pid.kd=q88_to_f(pkd);
        g_ctrl.spd_pid.kp=q88_to_f(skp); g_ctrl.spd_pid.ki=q88_to_f(ski); g_ctrl.spd_pid.kd=q88_to_f(skd);
      }
      break;

    case CMD_SET_PID_CURRENT:
      if(msg->payload_len>=4){
        int16_t ckp=(int16_t)((msg->payload[1]<<8)|msg->payload[0]);
        int16_t cki=(int16_t)((msg->payload[3]<<8)|msg->payload[2]);
        g_ctrl.cur_pi.kp=q88_to_f(ckp);
        g_ctrl.cur_pi.ki=q88_to_f(cki);
        g_ctrl.cur_pi.kd=0.0f;
      }
      break;

    case CMD_CAMERA_SNAPSHOT:
      if(msg->payload_len>=4){
        uint16_t d=(uint16_t)((msg->payload[1]<<8)|msg->payload[0]);
        uint16_t w=(uint16_t)((msg->payload[3]<<8)|msg->payload[2]);
        (void)camera_snapshot(d,w);
      }
      break;

    case CMD_GET_STATUS: {
      uint8_t frame[300];
      size_t n=telemetry_build_uart_frame(&g_ctx, frame, sizeof(frame));
      if(n) hal_uart_send(frame,n);
      break;
    }

    case CMD_CLEAR_FAULT: protection_clear(&g_ctx); break;
    default: break;
  }
}

static void task_ctrl_1khz(void){
  g_ctx.loop_counter++;

  sensor_process_1khz(&g_ctx);
  fsm_step_1khz(&g_ctx);
  protection_check_fast(&g_ctx);

  if((g_ctx.state==FSM_RUN || g_ctx.state==FSM_HOLD) && !protection_is_faulted(&g_ctx)){
    control_step_1khz(&g_ctrl, &g_ctx, 0.001f);
    g_ctx.motor_enable = true;
  } else {
    g_ctx.motor_enable = false;
    g_ctx.pwm_duty = 0;
  }

  hal_motor_enable(g_ctx.motor_enable && !protection_is_faulted(&g_ctx));
  hal_motor_set_duty(g_ctx.pwm_duty);
}

static void task_slow_100hz(void){ protection_check_slow(&g_ctx); }

static void task_telem_10hz(void){
  uint8_t frame[300];
  size_t n=telemetry_build_uart_frame(&g_ctx, frame, sizeof(frame));
  if(n) hal_uart_send(frame,n);
}

static void on_tick_1ms_isr(void){ scheduler_on_tick_isr(); }

bool app_init(void){
  memset(&g_ctx,0,sizeof(g_ctx));
  g_ctx.state=FSM_IDLE;

  if(!hal_init()) return false;
  protection_init(&g_ctx);
  fsm_init(&g_ctx);
  control_init(&g_ctrl,&g_ctx);

  cmd_parser_init(handle_cmd);

  if(!uart_drv_init(115200, on_uart_rx)) return false;
  if(!adc_drv_init(g_adc_dma_words, ARRAY_LEN(g_adc_dma_words), adc_half_cb, adc_full_cb)) return false;
  if(!timer_drv_init_1ms(on_tick_1ms_isr)) return false;
  if(!hal_adc_start()) return false;

  scheduler_init();
  scheduler_add_task(task_ctrl_1khz, TASK_CTRL_1KHZ_MS);
  scheduler_add_task(task_slow_100hz, TASK_SLOW_100HZ_MS);
  scheduler_add_task(task_telem_10hz, TASK_TELEM_10HZ_MS);
  return true;
}
