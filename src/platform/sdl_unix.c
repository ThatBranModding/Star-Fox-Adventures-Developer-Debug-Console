#include "platform.h"
#include <dlfcn.h>
#include <stddef.h>
#include <string.h>

typedef const unsigned char *(*SDL_GetKeyboardState_Fn)(int *numkeys);
typedef void *(*SDL_GetKeyboardFocus_Fn)(void);

static SDL_GetKeyboardState_Fn pGetKeyboardState;
static SDL_GetKeyboardFocus_Fn pGetKeyboardFocus;
static void *sdl_handle;
static unsigned char prev[512];
static int had_focus;

enum {
    SC_A = 4,
    SC_Z = 29,
    SC_1 = 30,
    SC_0 = 39,
    SC_RETURN = 40,
    SC_ESCAPE = 41,
    SC_BACKSPACE = 42,
    SC_SPACE = 44,
    SC_MINUS = 45,
    SC_SEMICOLON = 51,
    SC_COMMA = 54,
    SC_PERIOD = 55,
    SC_SLASH = 56,
    SC_F1 = 58,
    SC_HOME = 74,
    SC_PAGEUP = 75,
    SC_END = 77,
    SC_PAGEDOWN = 78,
    SC_DOWN = 81,
    SC_UP = 82,
    SC_LSHIFT = 225,
    SC_RSHIFT = 229
};

static void *lookup(const char *name) {
    void *p = dlsym(RTLD_DEFAULT, name);
    if (p)
        return p;
#if defined(__APPLE__)
    if (!sdl_handle) {
        const char *libs[] = {"libSDL3.dylib", "SDL3.framework/SDL3", NULL};
        int i;
        for (i = 0; libs[i] && !sdl_handle; ++i)
            sdl_handle = dlopen(libs[i], RTLD_LAZY | RTLD_LOCAL);
    }
#else
    if (!sdl_handle) {
        const char *libs[] = {"libSDL3.so.0", "libSDL3.so", NULL};
        int i;
        for (i = 0; libs[i] && !sdl_handle; ++i)
            sdl_handle = dlopen(libs[i], RTLD_LAZY | RTLD_LOCAL);
    }
#endif
    return sdl_handle ? dlsym(sdl_handle, name) : NULL;
}

static const unsigned char *keys(int *count) {
    if (!pGetKeyboardState) {
        if (count)
            *count = 0;
        return NULL;
    }
    return pGetKeyboardState(count);
}

static int focused(void) {
    return pGetKeyboardFocus && pGetKeyboardFocus() != NULL;
}

static void sync_state(void) {
    int n = 0, i;
    const unsigned char *k = keys(&n);
    memset(prev, 0, sizeof(prev));
    if (!k)
        return;
    if (n > (int)sizeof(prev))
        n = (int)sizeof(prev);
    for (i = 0; i < n; ++i)
        prev[i] = k[i] ? 1 : 0;
}

static int edge(const unsigned char *k, int n, int sc) {
    int down = (k && sc >= 0 && sc < n && k[sc]) ? 1 : 0;
    int e = down && !prev[sc];
    if (sc >= 0 && sc < (int)sizeof(prev))
        prev[sc] = (unsigned char)down;
    return e;
}

static char scancode_char(int sc, int shift) {
    if (sc >= SC_A && sc <= SC_Z)
        return (char)((shift ? 'A' : 'a') + (sc - SC_A));
    if (sc >= SC_1 && sc <= 38)
        return (char)('1' + (sc - SC_1));
    if (sc == SC_0)
        return '0';
    if (sc == SC_SPACE)
        return ' ';
    if (sc == SC_MINUS)
        return shift ? '_' : '-';
    if (sc == SC_PERIOD)
        return '.';
    if (sc == SC_COMMA)
        return ',';
    if (sc == SC_SEMICOLON)
        return shift ? ':' : ';';
    if (sc == SC_SLASH)
        return shift ? '?' : '/';
    return 0;
}

int platform_input_init(DevConsole *c) {
    (void)c;
    pGetKeyboardState = (SDL_GetKeyboardState_Fn)lookup("SDL_GetKeyboardState");
    pGetKeyboardFocus = (SDL_GetKeyboardFocus_Fn)lookup("SDL_GetKeyboardFocus");
    if (!pGetKeyboardState || !pGetKeyboardFocus)
        return 0;
    had_focus = focused();
    sync_state();
    return 1;
}

void platform_input_tick(DevConsole *c) {
    int n = 0, sc, shift;
    const unsigned char *k;
    int now_focus = focused();
    if (!now_focus) {
        if (had_focus)
            sync_state();
        had_focus = 0;
        return;
    }
    if (!had_focus) {
        sync_state();
        had_focus = 1;
        return;
    }
    k = keys(&n);
    if (!k)
        return;
    if (edge(k, n, SC_F1)) {
        console_toggle(c);
        return;
    }
    if (!c->open)
        return;
    if (edge(k, n, SC_ESCAPE)) {
        console_close(c);
        return;
    }
    if (edge(k, n, SC_BACKSPACE))
        console_backspace(c);
    if (edge(k, n, SC_RETURN))
        console_submit(c);
    if (edge(k, n, SC_UP))
        console_history_previous(c);
    if (edge(k, n, SC_DOWN))
        console_history_next(c);
    if (edge(k, n, SC_PAGEUP))
        console_scroll_up(c, DC_VISIBLE_LINES - 1);
    if (edge(k, n, SC_PAGEDOWN))
        console_scroll_down(c, DC_VISIBLE_LINES - 1);
    if (edge(k, n, SC_HOME))
        console_scroll_top(c);
    if (edge(k, n, SC_END))
        console_scroll_bottom(c);
    shift = (SC_LSHIFT < n && k[SC_LSHIFT]) || (SC_RSHIFT < n && k[SC_RSHIFT]);
    for (sc = SC_A; sc <= SC_Z; ++sc)
        if (edge(k, n, sc))
            console_append_char(c, scancode_char(sc, shift));
    for (sc = SC_1; sc <= SC_0; ++sc)
        if (edge(k, n, sc))
            console_append_char(c, scancode_char(sc, shift));
    {
        int extra[] = {SC_SPACE, SC_MINUS, SC_PERIOD, SC_COMMA, SC_SEMICOLON, SC_SLASH};
        int i;
        for (i = 0; i < 6; ++i)
            if (edge(k, n, extra[i]))
                console_append_char(c, scancode_char(extra[i], shift));
    }
}

void platform_input_shutdown(void) {
    if (sdl_handle)
        dlclose(sdl_handle);
    sdl_handle = NULL;
    pGetKeyboardState = NULL;
    pGetKeyboardFocus = NULL;
}
#if defined(__APPLE__)
const char *platform_input_backend_name(void) {
    return "macos-arm64-sdl3-focused-key-poll";
}
#else
const char *platform_input_backend_name(void) {
    return "linux-amd64-sdl3-focused-key-poll";
}
#endif
