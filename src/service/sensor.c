#include "service/sensor.h"
#include "service/temp_monitor.h"
#include "config.h"
#include "common.h"

static temp_monitor_t g_tm;
static bool g_tm_init = false;

static float adc_to_vref_mV(uint16_t raw_vref) {
  if (raw_vref == 0) raw_vref = 1;
  return 3300.0f * 4095.0f / (float)raw_vref;
}

static float adc_to_current_mA(uint16_t raw, float vref_mV) {
  (void)vref_mV;
  return (float)raw * 3000.0f / 4095.0f;
}

void sensor_on_adc_dma_block(system_ctx_t* ctx, const uint16_t* samples, uint32_t frames) {
  uint32_t sum[ADC_NUM_CHANNELS] = {0};
  for (uint32_t f = 0; f < frames; f++) {
    const uint16_t* frame = &samples[f * ADC_NUM_CHANNELS];
    for (uint32_t ch = 0; ch < ADC_NUM_CHANNELS; ch++) sum[ch] += frame[ch];
  }
  platform_enter_critical();
  for (uint32_t ch = 0; ch < ADC_NUM_CHANNELS; ch++) ctx->adc_raw[ch] = (uint16_t)(sum[ch] / frames);
  ctx->last_adc_ms = platform_millis();
  platform_exit_critical();
}

void sensor_process_1khz(system_ctx_t* ctx) {
  if (!g_tm_init) { temp_monitor_init(&g_tm); g_tm_init = true; }

  uint16_t raw_cur, raw_temp, raw_vref;
  platform_enter_critical();
  raw_cur  = ctx->adc_raw[ADC_CH_CURRENT_IDX];
  raw_temp = ctx->adc_raw[ADC_CH_TEMP_IDX];
  raw_vref = ctx->adc_raw[ADC_CH_VREF_IDX];
  platform_exit_critical();

  ctx->vref_mV = adc_to_vref_mV(raw_vref);
  ctx->temp_C  = temp_monitor_calc_C(&g_tm, raw_temp, raw_vref);
  ctx->current_mA = adc_to_current_mA(raw_cur, ctx->vref_mV);
}
