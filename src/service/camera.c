#include "service/camera.h"
#include "hal/hal_if.h"
bool camera_snapshot(uint16_t delay_us, uint16_t width_us){ return hal_camera_snapshot(delay_us,width_us); }
