#pragma once
#include "types.h"
void fsm_init(system_ctx_t* ctx);
void fsm_request_start(system_ctx_t* ctx);
void fsm_request_stop(system_ctx_t* ctx);
void fsm_step_1khz(system_ctx_t* ctx);
