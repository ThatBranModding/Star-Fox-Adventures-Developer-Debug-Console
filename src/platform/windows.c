#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "platform.h"

static int prev[256];
static int had_focus;

static int process_has_foreground(void) {
    HWND hwnd = GetForegroundWindow();
    DWORD pid = 0;
    if (!hwnd)
        return 0;
    GetWindowThreadProcessId(hwnd, &pid);
    return pid == GetCurrentProcessId();
}

static void sync_key_state(void) {
    int vk;
    for (vk = 0; vk < 256; ++vk)
        prev[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
}

static int edge(int vk) {
    int d = (GetAsyncKeyState(vk) & 0x8000) != 0;
    int e = d && !prev[vk];
    prev[vk] = d;
    return e;
}

int platform_input_init(DevConsole *c) {
    (void)c;
    ZeroMemory(prev, sizeof(prev));
    had_focus = process_has_foreground();
    sync_key_state();
    return 1;
}

static char keychar(int vk) {
    int shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    if (vk >= 'A' && vk <= 'Z')
        return shift ? (char)vk : (char)(vk + 32);
    if (vk >= '0' && vk <= '9')
        return (char)vk;
    if (vk == VK_SPACE)
        return ' ';
    if (vk == VK_OEM_MINUS)
        return shift ? '_' : '-';
    if (vk == VK_OEM_PERIOD)
        return '.';
    if (vk == VK_OEM_COMMA)
        return ',';
    if (vk == VK_OEM_1)
        return shift ? ':' : ';';
    if (vk == VK_OEM_2)
        return shift ? '?' : '/';
    return 0;
}

void platform_input_tick(DevConsole *c) {
    int vk;
    char ch;
    int focused = process_has_foreground();

    if (!focused) {
        if (had_focus)
            sync_key_state();
        had_focus = 0;
        return;
    }
    if (!had_focus) {
        sync_key_state();
        had_focus = 1;
        return;
    }

    if (edge(VK_F1)) {
        console_toggle(c);
        return;
    }
    if (!c->open)
        return;
    if (edge(VK_ESCAPE)) {
        console_close(c);
        return;
    }
    if (edge(VK_BACK))
        console_backspace(c);
    if (edge(VK_RETURN))
        console_submit(c);
    if (edge(VK_UP))
        console_history_previous(c);
    if (edge(VK_DOWN))
        console_history_next(c);
    if (edge(VK_PRIOR))
        console_scroll_up(c, DC_VISIBLE_LINES - 1);
    if (edge(VK_NEXT))
        console_scroll_down(c, DC_VISIBLE_LINES - 1);
    if (edge(VK_HOME))
        console_scroll_top(c);
    if (edge(VK_END))
        console_scroll_bottom(c);
    for (vk = 'A'; vk <= 'Z'; ++vk)
        if (edge(vk)) {
            ch = keychar(vk);
            if (ch)
                console_append_char(c, ch);
        }
    for (vk = '0'; vk <= '9'; ++vk)
        if (edge(vk)) {
            ch = keychar(vk);
            if (ch)
                console_append_char(c, ch);
        }
    {
        int extra[] = {VK_SPACE, VK_OEM_MINUS, VK_OEM_PERIOD, VK_OEM_COMMA, VK_OEM_1, VK_OEM_2};
        int i;
        for (i = 0; i < 6; ++i)
            if (edge(extra[i])) {
                ch = keychar(extra[i]);
                if (ch)
                    console_append_char(c, ch);
            }
    }
}

void platform_input_shutdown(void) {
}

const char *platform_input_backend_name(void) {
    return "windows-focused-key-poll";
}
