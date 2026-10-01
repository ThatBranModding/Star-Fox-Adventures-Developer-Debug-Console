#ifndef SFA_DEV_PLATFORM_H
#define SFA_DEV_PLATFORM_H
#include "../console.h"
int platform_input_init(DevConsole* c);
void platform_input_tick(DevConsole* c);
void platform_input_shutdown(void);
const char* platform_input_backend_name(void);
#endif
