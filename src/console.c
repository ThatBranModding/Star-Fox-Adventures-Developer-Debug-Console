#include "console.h"
#include <string.h>
#include <stdio.h>
static DevConsoleCommandHandler g_handler;
static int max_scroll(const DevConsole* c){int m=c->line_count-DC_VISIBLE_LINES;return m>0?m:0;}
void console_set_command_handler(DevConsoleCommandHandler h){g_handler=h;}
static void console_store_line(DevConsole* c,const char* s,size_t n){int i;if(c->line_count>=DC_MAX_LINES){for(i=1;i<DC_MAX_LINES;i++)memcpy(c->lines[i-1],c->lines[i],DC_LINE_LEN);c->line_count=DC_MAX_LINES-1;}if(n>=DC_LINE_LEN)n=DC_LINE_LEN-1;memcpy(c->lines[c->line_count],s,n);c->lines[c->line_count][n]=0;c->line_count++;c->scroll_offset=0;}
void console_push(DevConsole* c,const char* s){const char*p=s?s:"";const size_t wrap=DC_WRAP_CHARS;if(!*p){console_store_line(c,"",0);return;}while(*p){size_t remaining=strlen(p),cut;if(remaining<=wrap){console_store_line(c,p,remaining);break;}cut=wrap;while(cut>0&&p[cut]!=' '&&p[cut]!='\t')--cut;if(cut<wrap/2)cut=wrap;console_store_line(c,p,cut);p+=cut;while(*p==' '||*p=='\t')++p;}}
void console_init(DevConsole* c){memset(c,0,sizeof(*c));console_push(c,"SFA Developer Console 0.3.5");console_push(c,"Type 'help' for commands.");}
void console_toggle(DevConsole* c){c->open=!c->open;if(c->open)c->scroll_offset=0;}
void console_close(DevConsole* c){c->open=0;}
void console_append_char(DevConsole* c,char ch){if(c->input_len<DC_INPUT_LEN-1){c->input[c->input_len++]=ch;c->input[c->input_len]=0;c->scroll_offset=0;}}
void console_backspace(DevConsole* c){if(c->input_len>0)c->input[--c->input_len]=0;c->scroll_offset=0;}
void console_submit(DevConsole* c){char out[DC_LINE_LEN];int handled=0;if(!c->input_len)return;snprintf(out,sizeof(out),"> %s",c->input);console_push(c,out);if(!strcmp(c->input,"help")){console_push(c,"Commands: help, version, position");console_push(c,"teleport <place> [act]");console_push(c,"teleport boss <boss>");console_push(c,"infinite health");console_push(c,"infinite mana");console_push(c,"infinite tricky");console_push(c,"infinite status");handled=1;}else if(g_handler){handled=g_handler(c,c->input);}if(!handled){snprintf(out,sizeof(out),"Unknown command: %s",c->input);console_push(c,out);}c->input_len=0;c->input[0]=0;c->scroll_offset=0;}
void console_scroll_up(DevConsole* c,int lines){int m=max_scroll(c);c->scroll_offset+=lines;if(c->scroll_offset>m)c->scroll_offset=m;}
void console_scroll_down(DevConsole* c,int lines){c->scroll_offset-=lines;if(c->scroll_offset<0)c->scroll_offset=0;}
void console_scroll_top(DevConsole* c){c->scroll_offset=max_scroll(c);}
void console_scroll_bottom(DevConsole* c){c->scroll_offset=0;}
