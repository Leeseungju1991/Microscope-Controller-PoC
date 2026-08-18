#include "app/app_main.h"
#include "app/scheduler.h"
#include "platform/platform_posix.h"
#include "common.h"

int main(void){
  if(!app_init()){
    log_error("app_init failed");
    return 1;
  }
  while(1){
    platform_sim_step();
    scheduler_run();
  }
}
