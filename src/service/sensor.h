#pragma once
#include "types.h"
#include <stdint.h>
void sensor_on_adc_dma_block(system_ctx_t* ctx, const uint16_t* samples_words, uint32_t frames_in_block);
void sensor_process_1khz(system_ctx_t* ctx);
