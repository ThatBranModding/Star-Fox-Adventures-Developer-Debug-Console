#include "platform.h"
int platform_input_init(DevConsole* c){(void)c;return 1;}
void platform_input_tick(DevConsole* c){(void)c;}
void platform_input_shutdown(void){}
#if defined(__APPLE__)
const char* platform_input_backend_name(void){return "macos-arm64-pending";}
#else
const char* platform_input_backend_name(void){return "linux-amd64-pending";}
#endif
