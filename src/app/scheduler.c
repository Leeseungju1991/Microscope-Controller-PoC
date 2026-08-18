#include "app/scheduler.h"
#include "config.h"
typedef struct { task_fn_t fn; uint32_t period; uint32_t next; bool used; } task_t;
#define MAX_TASKS 12
static task_t g_tasks[MAX_TASKS];
static volatile uint32_t g_ms=0;
void scheduler_init(void){ for(int i=0;i<MAX_TASKS;i++) g_tasks[i].used=false; g_ms=0; }
void scheduler_on_tick_isr(void){ g_ms += SCHED_TICK_MS; }
static uint32_t now_ms(void){ return g_ms; }
bool scheduler_add_task(task_fn_t fn,uint32_t period_ms){
  for(int i=0;i<MAX_TASKS;i++){
    if(!g_tasks[i].used){
      g_tasks[i].used=true; g_tasks[i].fn=fn; g_tasks[i].period=period_ms; g_tasks[i].next=now_ms()+period_ms;
      return true;
    }
  }
  return false;
}
void scheduler_run(void){
  uint32_t n=now_ms();
  for(int i=0;i<MAX_TASKS;i++){
    if(!g_tasks[i].used) continue;
    if((int32_t)(n - g_tasks[i].next) >= 0){
      g_tasks[i].next += g_tasks[i].period;
      g_tasks[i].fn();
    }
  }
}
