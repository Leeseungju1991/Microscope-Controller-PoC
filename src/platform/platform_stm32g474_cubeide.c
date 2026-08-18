/*
 * STM32G474 + CubeIDE 연결 스켈레톤(메모)
 * - ADC1 DMA Circular, Half/Complete callback에서 adc_drv_isr_half/full 호출
 * - UART RX 1byte IT 또는 DMA ring -> uart_drv_feed_rx_byte 호출
 * - 1ms tick ISR -> scheduler_on_tick_isr() 호출
 * - Encoder TIM encoder mode -> encoder_drv_get_count() 구현
 * - Camera TIM one-pulse -> camera_trigger_fire(delay_us,width_us) 구현
 * - 온도 계산은 stm32g4xx_hal_adc_ex.h 매크로 사용 권장:
 *   vref_mV = __HAL_ADC_CALC_VREFANALOG_VOLTAGE(raw_vref, ADC_RESOLUTION_12B);
 *   temp_C  = __HAL_ADC_CALC_TEMPERATURE(vref_mV, raw_temp, ADC_RESOLUTION_12B);
 */
