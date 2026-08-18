#pragma once
#include "types.h"
#include <stddef.h>
#include <stdint.h>
size_t telemetry_build_uart_frame(const system_ctx_t* ctx, uint8_t* out, size_t out_cap);
