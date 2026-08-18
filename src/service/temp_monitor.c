#include "service/temp_monitor.h"
void temp_monitor_init(temp_monitor_t* tm) { tm->last_temp_C = 25.0f; }
static float clampf(float v, float lo, float hi){ if(v<lo)return lo; if(v>hi)return hi; return v; }
float temp_monitor_calc_C(temp_monitor_t* tm, uint16_t raw_temp, uint16_t raw_vref) {
  // PC sim approximation. For STM32G474 production:
  // vref_mV = __HAL_ADC_CALC_VREFANALOG_VOLTAGE(raw_vref, ADC_RESOLUTION_12B);
  // temp_C  = __HAL_ADC_CALC_TEMPERATURE(vref_mV, raw_temp, ADC_RESOLUTION_12B);
  if (raw_vref == 0) raw_vref = 1;
  float vref_mV = 3300.0f * 4095.0f / (float)raw_vref;
  float vtemp_mV = (float)raw_temp * (vref_mV / 4095.0f);
  float temp_C = (vtemp_mV - 760.0f) / 2.5f + 25.0f;
  temp_C = clampf(temp_C, -40.0f, 150.0f);
  tm->last_temp_C = temp_C;
  return temp_C;
}
