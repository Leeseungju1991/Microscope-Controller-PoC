#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef void (*adc_dma_half_cb_t)(void);
typedef void (*adc_dma_full_cb_t)(void);
bool adc_drv_init(uint16_t* dma_buf_words, size_t dma_buf_len_words, adc_dma_half_cb_t half_cb, adc_dma_full_cb_t full_cb);
bool adc_drv_start(void);
void adc_drv_stop(void);
void adc_drv_isr_half(void);
void adc_drv_isr_full(void);
