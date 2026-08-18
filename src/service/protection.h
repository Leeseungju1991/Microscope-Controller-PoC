#pragma once
#include "types.h"
#include <stdbool.h>
void protection_init(system_ctx_t* ctx);
void protection_check_fast(system_ctx_t* ctx);
void protection_check_slow(system_ctx_t* ctx);
bool protection_is_faulted(const system_ctx_t* ctx);
void protection_clear(system_ctx_t* ctx);
