#include "common.h"
#include "config.h"
#include "driver/uart_drv.h"
#include "driver/adc_drv.h"
#include "driver/motor_drv.h"
#include "driver/timer_drv.h"
#include "driver/encoder_drv.h"
#include "driver/camera_trigger_drv.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// logging
static void vlog(const char* t,const char* fmt,va_list ap){ fprintf(stderr,"[%s] ",t); vfprintf(stderr,fmt,ap); fprintf(stderr,"\n"); }
void log_info(const char* fmt,...){ va_list ap;va_start(ap,fmt);vlog("I",fmt,ap);va_end(ap); }
void log_warn(const char* fmt,...){ va_list ap;va_start(ap,fmt);vlog("W",fmt,ap);va_end(ap); }
void log_error(const char* fmt,...){ va_list ap;va_start(ap,fmt);vlog("E",fmt,ap);va_end(ap); }

// time
static uint64_t mono_ms(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return (uint64_t)ts.tv_sec*1000ull + (uint64_t)ts.tv_nsec/1000000ull; }
static uint64_t t0=0;
uint32_t platform_millis(void){ if(!t0)t0=mono_ms(); return (uint32_t)(mono_ms()-t0); }
void platform_enter_critical(void){}
void platform_exit_critical(void){}

// UART
static uart_rx_cb_t g_rx=0;
bool uart_drv_init(uint32_t baud, uart_rx_cb_t cb){ (void)baud; g_rx=cb; log_info("UART init(sim)"); return true; }
bool uart_drv_tx(const uint8_t* d, size_t n){
  printf("TX(%zu):",n);
  for(size_t i=0;i<n;i++) printf(" %02X", d[i]);
  printf("\n"); fflush(stdout);
  return true;
}
void uart_drv_feed_rx_byte(uint8_t b){ if(g_rx) g_rx(b); }

// ADC DMA
static adc_dma_half_cb_t g_half=0;
static adc_dma_full_cb_t g_full=0;
static uint16_t* g_buf=0;
static size_t g_words=0;
static bool g_run=false;
bool adc_drv_init(uint16_t* buf, size_t words, adc_dma_half_cb_t h, adc_dma_full_cb_t f){ g_buf=buf; g_words=words; g_half=h; g_full=f; log_info("ADC init(sim) words=%zu",words); return true; }
bool adc_drv_start(void){ g_run=true; return true; }
void adc_drv_stop(void){ g_run=false; }
void adc_drv_isr_half(void){ if(g_half) g_half(); }
void adc_drv_isr_full(void){ if(g_full) g_full(); }

// Motor
static bool g_en=false;
static uint16_t g_duty=0;   // last commanded PWM duty (0..MOTOR_PWM_MAX_DUTY)
bool motor_drv_init(void){ log_info("Motor init(sim)"); return true; }
void motor_drv_set_enable(bool en){ if(en!=g_en){ g_en=en; log_info("Motor enable=%d",(int)en);} }
void motor_drv_set_pwm_duty(uint16_t d){ g_duty=d; }

// Encoder
static int32_t g_cnt=0;
bool encoder_drv_init(void){ log_info("Encoder init(sim)"); return true; }
bool encoder_drv_start(void){ return true; }
int32_t encoder_drv_get_count(void){ return g_cnt; }

// Camera one pulse
bool camera_trigger_drv_init(void){ log_info("Camera trigger init(sim)"); return true; }
bool camera_trigger_fire(uint16_t delay_us, uint16_t width_us){ log_info("Camera snapshot delay=%u width=%u", delay_us, width_us); return true; }

// Timer tick
static timer_tick_cb_t g_tick=0;
bool timer_drv_init_1ms(timer_tick_cb_t cb){ g_tick=cb; log_info("Timer 1ms init(sim)"); return true; }

// Produce calibrated, in-range ADC samples so the simulator runs healthy
// (no spurious over-current/over-temp faults) and the current loop visibly
// tracks the commanded PWM duty.
//
// Calibration matches service/sensor.c + service/temp_monitor.c:
//   vref_mV  = 3300 * 4095 / raw_vref          -> raw_vref=4095 => 3300mV
//   current  = raw_cur * 3000 / 4095  [mA]
//   temp_C   = (raw_temp*vref_mV/4095 - 760)/2.5 + 25
static void fill_adc(uint32_t t_ms){
  if(!g_run || !g_buf) return;

  // VREFINT: raw that yields ~3300mV in the sim formula.
  const uint16_t vref = 4095u;

  // Coil temperature ~ 35C steady (raw ~974), with a slow +/-1C ripple.
  // (raw_temp -> ~785mV -> ~35C; well below PROT_OVERTEMP_C=85C)
  uint16_t temp = (uint16_t)(974 + (int)(4.0 * ((t_ms/200u)%5u)) - 8);

  // Motor current models a first-order response to commanded duty:
  //   I = quiescent + (duty/MAX) * full-scale, plus small ripple.
  // Stays inside CURRENT_SP_MAX_mA / PROT_OVERCURRENT_mA(2500mA).
  static float i_mA = 0.0f;
  float i_target_mA = g_en ? (120.0f + (float)g_duty / (float)MOTOR_PWM_MAX_DUTY * 1500.0f)
                           : 60.0f;
  i_mA += (i_target_mA - i_mA) * 0.05f;                  // tau ~ 20ms
  float ripple = 20.0f * (float)((t_ms % 7u)) / 7.0f;    // small noise
  uint16_t cur = (uint16_t)((i_mA + ripple) * 4095.0f / 3000.0f);

  uint16_t res = (uint16_t)((t_ms*3) % 4096);            // unused reserved ch

  const uint32_t frames = (uint32_t)(g_words / ADC_NUM_CHANNELS);
  for(uint32_t f=0; f<frames; f++){
    uint16_t* frame = &g_buf[f*ADC_NUM_CHANNELS];
    frame[0]=cur; frame[1]=res; frame[2]=temp; frame[3]=vref;
  }

  // Encoder advances only while the motor is enabled (rough feedback model).
  if(g_en) g_cnt += 2;

  if((t_ms%2)==0) adc_drv_isr_half(); else adc_drv_isr_full();
}

void platform_sim_step(void){
  static uint32_t last=0;
  if(last==0) last=platform_millis();
  uint32_t now=platform_millis();
  while((now-last)>=1){
    last += 1;
    fill_adc(last);
    if(g_tick) g_tick();
  }
}
