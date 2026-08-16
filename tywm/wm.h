#ifndef WM_H
#define WM_H

#include <stdbool.h>
#include <stddef.h>
#include <X11/Xlib.h>

extern int BAR_HEIGHT;

typedef struct {
    Window window;

    char title[256];

    int x;
    int y;

    unsigned int width;
    unsigned int height;

    bool floating;
    bool focused;
    bool resising;
    bool fullscrened;
    int mapped;
    int workspace;
} Client;



typedef struct {
    char terminal[256];
    char launcher[256];

    char bar_color[32];
    int bar_height;
    bool bar_visible;

    char background_wallpaper[512];

    char autoexec[6][512];

    int border_width;
    char border_color[32];

    char float_keybind[32];
    char terminal_keybind[32];
    char kill_keybind[32];
    char focus_next_keybind[32];
    char unfloat_keybind[32];
    char toggle_bar_keybind[32];
    char reload_key[32];
    int tile_mode;
    char launcher_keybind[32];

} Config;

extern Config config;


extern bool bar_enabled;

extern Window bar_window;
extern int current_workspace;

extern Client clients[];
extern int client_count;


Client *get_focused_client(void);

void tile(Display *dpy, Client *clients, int client_count, int tile_mode);
void focus_client(Display *dpy, Client *c);
void SetupColors(Display *dpy);
void float_client(Display *dpy, Client *c, int x, int y,
                  unsigned int width, unsigned int height);
void get_window_title(Display *dpy, Window w, char *title, size_t size);
void unfloat_client(Display *dpy, Client *c);
void switch_workspace(Display *dpy, int target_workspace);
int is_dialog(Display *dpy, Window w);
void move_to_workspace(Display *dpy, Client *c, int target_workspace);
bool parse_bool(char *value);
void load_config();
void set_default_config();

#endif
