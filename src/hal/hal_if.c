#include "hal/hal_if.h"
#include "driver/adc_drv.h"
#include "driver/uart_drv.h"
#include "driver/motor_drv.h"
#include "driver/encoder_drv.h"
#include "driver/camera_trigger_drv.h"

bool hal_init(void) {
  if (!motor_drv_init()) return false;
  if (!encoder_drv_init()) return false;
  if (!camera_trigger_drv_init()) return false;
  return true;
}
bool hal_adc_start(void) { return adc_drv_start(); }
void hal_adc_stop(void) { adc_drv_stop(); }
bool hal_uart_send(const uint8_t* data, size_t len) { return uart_drv_tx(data, len); }
void hal_motor_enable(bool en) { motor_drv_set_enable(en); }
void hal_motor_set_duty(uint16_t duty) { motor_drv_set_pwm_duty(duty); }
int32_t hal_encoder_get_count(void) { return encoder_drv_get_count(); }
bool hal_camera_snapshot(uint16_t delay_us, uint16_t width_us) { return camera_trigger_fire(delay_us, width_us); }
