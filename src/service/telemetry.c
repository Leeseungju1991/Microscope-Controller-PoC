#include "service/telemetry.h"
#include "service/cmd_parser.h"
#include <stdio.h>

size_t telemetry_build_uart_frame(const system_ctx_t* ctx, uint8_t* out, size_t out_cap){
  char json[240];
  int n = snprintf(json, sizeof(json),
    "{\"st\":%d,\"flt\":%d,\"pos\":%.4f,\"tpos\":%.4f,\"rpm\":%.1f,\"trpm\":%.1f,"
    "\"ma\":%.1f,\"tma\":%.1f,\"tc\":%.1f,\"d\":%u,\"cnt\":%lu}",
    (int)ctx->state,(int)ctx->fault,
    (double)ctx->position_rev,(double)ctx->target_position_rev,
    (double)ctx->speed_rpm,(double)ctx->target_speed_rpm,
    (double)ctx->current_mA,(double)ctx->target_current_mA,
    (double)ctx->temp_C,
    (unsigned)ctx->pwm_duty,(unsigned long)ctx->loop_counter
  );
  if(n<0 || n>=(int)sizeof(json)) return 0;
  return cmd_build_frame(0x84, (const uint8_t*)json, (uint8_t)n, out, out_cap);
}
