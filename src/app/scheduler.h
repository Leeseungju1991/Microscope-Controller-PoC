#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef void (*task_fn_t)(void);
void scheduler_init(void);
void scheduler_on_tick_isr(void);
void scheduler_run(void);
bool scheduler_add_task(task_fn_t fn, uint32_t period_ms);
