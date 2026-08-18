#include "service/protection.h"
#include "config.h"
#include "common.h"

static void set_fault(system_ctx_t* ctx, fault_code_t code){
  if(ctx->fault==FAULT_NONE){
    ctx->fault=code;
    ctx->state=FSM_FAULT;
    ctx->motor_enable=false;
    ctx->pwm_duty=0;
    log_error("FAULT=%d",(int)code);
  }
}
void protection_init(system_ctx_t* ctx){ ctx->fault=FAULT_NONE; }
void protection_check_fast(system_ctx_t* ctx){
  if(ctx->current_mA > (float)PROT_OVERCURRENT_mA) set_fault(ctx, FAULT_OVERCURRENT);
  if(ctx->speed_rpm > (float)PROT_OVERSPEED_RPM)  set_fault(ctx, FAULT_OVERSPEED);
  if(ctx->temp_C > (float)PROT_OVERTEMP_C)        set_fault(ctx, FAULT_OVERTEMP);
}
void protection_check_slow(system_ctx_t* ctx){
  uint32_t now = platform_millis();
  if((now-ctx->last_adc_ms) > PROT_SENSOR_STALE_MS) set_fault(ctx, FAULT_SENSOR_STALE);
  if(ctx->state==FSM_RUN){
    if((now-ctx->last_cmd_ms) > PROT_UART_TIMEOUT_MS) set_fault(ctx, FAULT_UART_TIMEOUT);
  }
}
bool protection_is_faulted(const system_ctx_t* ctx){ return ctx->fault!=FAULT_NONE; }
void protection_clear(system_ctx_t* ctx){ ctx->fault=FAULT_NONE; if(ctx->state==FSM_FAULT) ctx->state=FSM_IDLE; }
