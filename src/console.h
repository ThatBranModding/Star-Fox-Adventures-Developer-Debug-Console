#ifndef SFA_DEV_CONSOLE_H
#define SFA_DEV_CONSOLE_H
#define DC_MAX_LINES 128
#define DC_LINE_LEN 160
#define DC_INPUT_LEN 128
#define DC_VISIBLE_LINES 6
#define DC_WRAP_CHARS 46

typedef struct DevConsole {
 int open;
 char input[DC_INPUT_LEN];
 int input_len;
 char lines[DC_MAX_LINES][DC_LINE_LEN];
 int line_count;
 int scroll_offset;
} DevConsole;
typedef int (*DevConsoleCommandHandler)(DevConsole* c,const char* command);
void console_init(DevConsole* c);
void console_set_command_handler(DevConsoleCommandHandler handler);
void console_toggle(DevConsole* c);
void console_close(DevConsole* c);
void console_append_char(DevConsole* c,char ch);
void console_backspace(DevConsole* c);
void console_submit(DevConsole* c);
void console_push(DevConsole* c,const char* s);
void console_scroll_up(DevConsole* c,int lines);
void console_scroll_down(DevConsole* c,int lines);
void console_scroll_top(DevConsole* c);
void console_scroll_bottom(DevConsole* c);
#endif
