#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef ARRAY_LEN
#define ARRAY_LEN(x) (sizeof(x)/sizeof((x)[0]))
#endif

void log_info(const char* fmt, ...);
void log_warn(const char* fmt, ...);
void log_error(const char* fmt, ...);

uint32_t platform_millis(void);
void platform_enter_critical(void);
void platform_exit_critical(void);
