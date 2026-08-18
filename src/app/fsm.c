#include "app/fsm.h"
#include "service/protection.h"
void fsm_init(system_ctx_t* ctx){ ctx->state=FSM_IDLE; ctx->motor_enable=false; ctx->pwm_duty=0; }
void fsm_request_start(system_ctx_t* ctx){
  if(protection_is_faulted(ctx)) return;
  if(ctx->state==FSM_IDLE || ctx->state==FSM_HOLD){ ctx->state=FSM_RUN; ctx->motor_enable=true; }
}
void fsm_request_stop(system_ctx_t* ctx){ ctx->state=FSM_IDLE; ctx->motor_enable=false; ctx->pwm_duty=0; }
void fsm_step_1khz(system_ctx_t* ctx){
  switch(ctx->state){
    case FSM_IDLE: ctx->motor_enable=false; ctx->pwm_duty=0; break;
    case FSM_RUN: break;
    case FSM_HOLD: break;
    case FSM_FAULT: ctx->motor_enable=false; ctx->pwm_duty=0; break;
    case FSM_CAL: break;
    default: ctx->state=FSM_FAULT; ctx->fault=FAULT_INTERNAL; break;
  }
}
