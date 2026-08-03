#include <X11/X.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "bar.h"
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include "configuration.h"
#include "wm.h"
#include <X11/keysym.h>
#include <signal.h>
#include <stdint.h>
#include <X11/XKBlib.h>


int running = 1;



bool bar_enabled = bar_visible;

void spawn(const char *cmd);


static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

unsigned long focused_color;
unsigned long unfocused_color;
unsigned long bar_color_pixel;
unsigned long keybind_match_float_keybind;

void grabKey(const char *key, unsigned int mod, Display *dpy, Window root)
{
    KeySym sym = XStringToKeysym(key);
    KeyCode code = XKeysymToKeycode(dpy, sym);

    unsigned int lock_masks[] = { 0, LockMask, Mod2Mask, LockMask | Mod2Mask };

    for (int i = 0; i < 4; i++) {
        XGrabKey(
            dpy, code, mod | lock_masks[i], root,
            false, GrabModeAsync, GrabModeAsync
        );
    }

    XSync(dpy, False);
}






Client *get_focused_client(void);



#define MAX_CLIENTS 64
#define MAX_WORKSPACES 9

Client clients[MAX_CLIENTS];
int client_count = 0;

int wm_detected = 0;

Client *drag_client = NULL;
int drag_start_x, drag_start_y;
int drag_orig_x, drag_orig_y;
unsigned int drag_orig_w, drag_orig_h;
int dragging_resize = 0;

int xerror(Display *dpy, XErrorEvent *e)
{
    if (e->error_code == BadAccess)
        wm_detected = 1;

    return 0;
}


int spawn_wallpaper(const char *cmd) {
    if (fork() == 0) {
        setsid();
        execvp("feh", (char *[]){"feh", "--bg-fill", background_wallpaper, NULL});
        _exit(1);
    }
    return 0;
}


void autoexecs() {
    if (autoexec1[0] != '\0') spawn(autoexec1);
    if (autoexec2[0] != '\0') spawn(autoexec2);
    if (autoexec3[0] != '\0') spawn(autoexec3);
    if (autoexec4[0] != '\0') spawn(autoexec4);
    if (autoexec5[0] != '\0') spawn(autoexec5);
    if (autoexec6[0] != '\0') spawn(autoexec6);
}

GC gc;
Window bar_window;
int current_workspace = 0;

int main() {
    printf("i fucking hate c");
    unsetenv("WAYLAND_DISPLAY");
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGCHLD, SIG_IGN);
    Display *dpy = XOpenDisplay(NULL);
  
    if (dpy == NULL) {
        fprintf(stderr, "Unable to open display\n");
        return 1;
    } 
    int xkb_supported;
    XkbSetDetectableAutoRepeat(dpy, True, &xkb_supported);
    printf("Display opened successfully\n");

    Window root = DefaultRootWindow(dpy);
    


    

    XSetErrorHandler(xerror);

    XSelectInput(
        dpy,
        root,
        SubstructureRedirectMask |
        SubstructureNotifyMask |
        ButtonPressMask |
        KeyPressMask |
        EnterWindowMask |
        FocusChangeMask
    );

    XGrabButton(
        dpy,
        Button1,              // left click — move
        Mod1Mask,             // Alt+drag
        root,
        True,                 // report events relative to grab window
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None
    );

    XGrabButton(
        dpy,
        Button3,              // right click — resize
        Mod1Mask,             // Alt+drag
        root,
        True,
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None
    );

    XSync(dpy, False);

    if (wm_detected) {
        fprintf(stderr, "Another window manager is already running\n");
        return 1;
    }

    SetupColors(dpy);


    bar_window = XCreateSimpleWindow(
        dpy,
        root,
        0,
        0,
        DisplayWidth(dpy, DefaultScreen(dpy)),
        BAR_HEIGHT,
        0,
        BlackPixel(dpy, DefaultScreen(dpy)),
        bar_color_pixel
    );
    
    XSelectInput(dpy, bar_window, ExposureMask);
    XMapRaised(dpy, bar_window);


    
    XGCValues gcv;
    gcv.foreground = WhitePixel(dpy, DefaultScreen(dpy));
    gc = XCreateGC(dpy, bar_window, GCForeground, &gcv);

    printf("window manager started\n");

    Cursor cursor = XCreateFontCursor(dpy, XC_left_ptr);
    XDefineCursor(dpy, root, cursor);


    /*XGrabButton(
        dpy,
        Button1,
        0,
        root,
        False,
        ButtonPressMask,
        GrabModeSync,
        GrabModeAsync,
        None,
        None
    );
    */
    
    grabKey(terminal_keybind, Mod4Mask, dpy, root);
    grabKey(float_keybind, Mod1Mask, dpy, root);
    grabKey(kill_keybind, Mod1Mask, dpy, root);
    grabKey(focus_next_keybind, Mod1Mask, dpy, root);
    grabKey(unfloat_keybind, Mod1Mask, dpy, root);
    grabKey(toggle_bar_keybind, Mod1Mask, dpy, root);
    grabKey(launcher_keybind, Mod1Mask, dpy, root);

    for (int i = 0; i < 9; i++) {
        char key[2] = { '1' + i, '\0' };
        grabKey(key, Mod1Mask, dpy, root);
        grabKey(key, Mod1Mask | ShiftMask, dpy, root);
    }
  
    spawn_wallpaper(background_wallpaper);
    autoexecs();
    while (running) {
        XEvent event;
        XNextEvent(dpy, &event);

        switch (event.type) {

        case KeyPress:

            if (event.xkey.state & Mod4Mask &&
                event.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym(terminal_keybind)))
            {
                spawn(terminal);
            }

            if (event.xkey.state & Mod1Mask &&
                event.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym(kill_keybind)))
            {
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].focused) {
                        XKillClient(dpy, clients[i].window);
                        break;
                    }
                }
            }

            if (event.xkey.state & Mod1Mask &&
                event.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym(float_keybind)))
            {
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].focused) {
                        clients[i].floating = !clients[i].floating;
                        float_client(dpy, &clients[i], 100, 100, 800, 600);
                        break;
                    }
                }
            }

            if ((event.xkey.state & Mod1Mask) &&
                    (event.xkey.state & ShiftMask)) {
                for (int i = 0; i < 9; i++) {
                    char key[2] = { '1' + i, '\0' };
                    KeySym sym = XStringToKeysym(key);
                    if (event.xkey.keycode == XKeysymToKeycode(dpy, sym)) {
                        Client *c = get_focused_client();
                    if (c)
                        move_to_workspace(dpy, c, i);
                            break;
                    }
                }
            }

            if ((event.xkey.state & Mod1Mask) &&
                !(event.xkey.state & ShiftMask)) {
                for (int i = 0; i < 9; i++) {
                    char key[2] = { '1' + i, '\0' };
                    KeySym sym = XStringToKeysym(key);
                    if (event.xkey.keycode == XKeysymToKeycode(dpy, sym)) {
                        switch_workspace(dpy, i);
                        break;
                    }
                }
            }

            if (event.xkey.state & Mod1Mask &&
                event.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym(toggle_bar_keybind)))
            {
                if (bar_enabled) {
                    hide_bar(dpy);
                } else {
                    show_bar(dpy);
                }
            }

            if (event.xkey.state & Mod1Mask &&
                event.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym(unfloat_keybind)))
            {
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].focused) {
                        unfloat_client(dpy, &clients[i]);
                        break;
                    }
                }
            }

            if (event.xkey.state & Mod1Mask &&
                event.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym(launcher_keybind)))
            {
                spawn(launcher);
            }

            if (event.xkey.state & Mod1Mask &&
                event.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym(focus_next_keybind)))
            {
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].focused) {
                        int next_index = (i + 1) % client_count;
                        int tries = 0;
                        while (!clients[next_index].mapped && tries < client_count) {
                            next_index = (next_index + 1) % client_count;
                            tries++;
                        }
                        if (tries >= client_count)
                            break;
                        focus_client(dpy, &clients[next_index]);
   
                        break;
                    }
                }    
            }



            

            break;

        case MapRequest:
            {
                Window w = event.xmaprequest.window;

                XWindowAttributes attr;
                XGetWindowAttributes(dpy, w, &attr);

                bool dialog = is_dialog(dpy, w);

                Window transient;
                bool has_transient = XGetTransientForHint(dpy, w, &transient);
          
                if (client_count >= MAX_CLIENTS)
                    break;

                Client *c = &clients[client_count++];

                XSelectInput(
                    dpy,
                    w,
                    EnterWindowMask |
                    FocusChangeMask |
                    PropertyChangeMask
                );

                c->window = w;
                c->mapped = 0;
                c->focused = false;
                c->floating = dialog || has_transient;
                c->workspace = current_workspace;

                c->x = attr.x;
                c->y = attr.y;
                c->width = attr.width;
                c->height = attr.height;

                XSetWindowBorderWidth(dpy, w, border_width);

                get_window_title(
                    dpy,
                    w,
                    c->title,
                    sizeof(c->title)
                );

                XMapRaised(dpy, w);
                c->mapped = 1;
                if (c->floating) {
                    int sw = DisplayWidth(dpy, DefaultScreen(dpy));
                    int sh = DisplayHeight(dpy, DefaultScreen(dpy));

                    int nx = (sw - c->width) / 2;
                    int ny = (sh - c->height) / 2;

                    XMoveWindow(dpy, w, nx, ny);

                    c->x = nx;
                    c->y = ny;
                }

                tile(dpy, clients, client_count);

                focus_client(dpy, c);

                XFlush(dpy);

                break;
            }

        case Expose:
        if (event.xexpose.window == bar_window) {
            draw_bar(dpy, bar_window, gc);
        }
    break;

        case DestroyNotify:
        {
            Window w = event.xdestroywindow.window;
            int was_focused = 0;

            for (int i = 0; i < client_count; i++) {
                if (clients[i].window == w) {
                    was_focused = clients[i].focused;
                    for (int j = i; j < client_count - 1; j++)
                        clients[j] = clients[j + 1];
                    client_count--;
                    break;
                }
            }

            if (was_focused && client_count > 0)
                focus_client(dpy, &clients[0]);   // or pick "next mapped" if you prefer

            tile(dpy, clients, client_count);
            break;
        }
            break;
        case UnmapNotify:
        {
           if (event.xunmap.window == root)
                break;
            Window w = event.xunmap.window;
            int was_focused = 0;

            for (int i = 0; i < client_count; i++) {
                if (clients[i].window == w) {
                    clients[i].mapped = 0;
                    was_focused = clients[i].focused;
                    break;
                }
            }

            if (was_focused) {
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].mapped) {
                        focus_client(dpy, &clients[i]);
                        break;
                    }
                }
            }

            tile(dpy, clients, client_count);
            break;
        }
       case ConfigureRequest:
            {
                XConfigureRequestEvent *e = &event.xconfigurerequest;

                Client *c = NULL;

                for (int i = 0; i < client_count; i++) {
                    if (clients[i].window == e->window) {
                        c = &clients[i];
                        break;
                    }
                }

                if (c && !c->floating) {
                    tile(dpy, clients, client_count);
                }
                else {
                    XWindowChanges changes;

                    changes.x = e->x;
                    changes.y = e->y;
                    changes.width = e->width;
                    changes.height = e->height;
                    

                    XConfigureWindow(
                        dpy,
                        e->window,
                        e->value_mask,
                        &changes
                    );
                }

                break;
            }

        case EnterNotify:
            {
                Window w = event.xcrossing.window;

                for (int i = 0; i < client_count; i++) {
                    if (clients[i].window == w) {
                        focus_client(dpy, &clients[i]);
                        break;
                    }
                }

                break;
            }

            case ButtonPress:
            {
                Window w = event.xbutton.subwindow;
                if (w == None) break;

                Client *c = NULL;
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].window == w) { c = &clients[i]; break; }
                }
                if (!c) break;

                // dragging only makes sense on floating windows
                if (!c->floating) {
                    c->floating = true;
                    float_client(dpy, c, c->x, c->y, c->width, c->height);
                }

                focus_client(dpy, c);

                drag_client = c;
                drag_start_x = event.xbutton.x_root;
                drag_start_y = event.xbutton.y_root;
                drag_orig_x = c->x;
                drag_orig_y = c->y;
                drag_orig_w = c->width;
                drag_orig_h = c->height;
                dragging_resize = (event.xbutton.button == Button3);

                break;
            }

            case PropertyNotify:
            {
                Window w = event.xproperty.window;

                for (int i = 0; i < client_count; i++)
                {
                    if (clients[i].window == w)
                    {
                        get_window_title(
                            dpy,
                            w,
                            clients[i].title,
                            sizeof(clients[i].title)
                        );

                        draw_bar(dpy, bar_window, gc);
                        break;
                    }
                }

                break;
            }

            case MotionNotify:
            {
                if (!drag_client) break;

                // drain extra motion events so drag doesn't lag behind the cursor
                while (XCheckTypedEvent(dpy, MotionNotify, &event));

                int dx = event.xmotion.x_root - drag_start_x;
                int dy = event.xmotion.y_root - drag_start_y;

                if (dragging_resize) {
                    unsigned int new_w = drag_orig_w + dx > 20 ? drag_orig_w + dx : 20; //i dont even know what the fuck this does
                    unsigned int new_h = drag_orig_h + dy > 20 ? drag_orig_h + dy : 20;
                    drag_client->width = new_w;
                    drag_client->height = new_h;
                    XResizeWindow(dpy, drag_client->window, new_w, new_h);
                } else {
                    int new_x = drag_orig_x + dx;
                    int new_y = drag_orig_y + dy;
                    drag_client->x = new_x;
                    drag_client->y = new_y;
                    XMoveWindow(dpy, drag_client->window, new_x, new_y);
                }

                break;
            }

            case ButtonRelease:
            {
                drag_client = NULL;
                break;
            }
        
        }


    }
}




void tile(Display *dpy, Client *clients, int client_count) {

    int visible_count = 0;
    for (int i = 0; i < client_count; i++) {
        if (clients[i].mapped && !clients[i].floating &&
            clients[i].workspace == current_workspace)
        {
            visible_count++;
        }
    }

    if (visible_count == 0)
        return;

    int width = DisplayWidth(dpy, DefaultScreen(dpy)) / visible_count;
    int bar_offset = custom_bar;

    int height = DisplayHeight(
        dpy,
        DefaultScreen(dpy)
    ) - bar_offset;

    int slot = 0;
    for (int i = 0; i < client_count; i++) {
        Client *c = &clients[i];
        if (c->mapped && !c->floating && c->workspace == current_workspace) {
            int bw = border_width;

            XMoveResizeWindow(
                dpy,
                c->window,
                slot * width,
                bar_offset,
                width - 2 * bw,
                height - 2 * bw
            );
            slot++;
        }
    }
}


void float_client(Display *dpy, Client *c, int x, int y, unsigned int width, unsigned int height)
{
    if (!c || !c->mapped)
        return;

    c->floating = true;

    XMoveResizeWindow(dpy, c->window, x, y, width, height);
    XFlush(dpy);

    if (c->focused) {
        XRaiseWindow(dpy, c->window);
    }
}


void resising_client(Display *dpy, Client *c, int x, int y, unsigned int width, unsigned int height)
{
    if (!c || !c->mapped)
        return;

    c->resising = true;

    XMoveResizeWindow(dpy, c->window, x, y, width, height);
    XFlush(dpy);

    if (c->focused) {
        XRaiseWindow(dpy, c->window);
    }
}

void unfloat_client(Display *dpy, Client *c)
{
    if (!c || !c->mapped)
        return;

    c->floating = false;

    tile(dpy, clients, client_count);
    XFlush(dpy);
}

void focus_client(Display *dpy, Client *c)
{
    if (!c || !c->mapped)
        return;

    for (int i = 0; i < client_count; i++) {
        clients[i].focused = false;

        XSetWindowBorder(
            dpy,
            clients[i].window,
            unfocused_color
        );
    }

    c->focused = true;
    

    XSetWindowBorderWidth(dpy, c->window, border_width);

    XSetWindowBorder(
        dpy,
        c->window,
        focused_color
    );

    XRaiseWindow(dpy, c->window);

    XSetInputFocus(
        dpy,
        c->window,
        RevertToPointerRoot,
        CurrentTime
    );

    XFlush(dpy);
}
void spawn(const char *cmd) {
    if (fork() == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(1);
    }
}

void switch_workspace(Display *dpy, int target_workspace)
{
    if (target_workspace == current_workspace)
        return;

    int old_workspace = current_workspace;
    current_workspace = target_workspace;

    for (int i = 0; i < client_count; i++) {
        Client *c = &clients[i];

        if (c->workspace == target_workspace) {
            XMapWindow(dpy, c->window);
            c->mapped = 1;
        }
        else if (c->workspace == old_workspace) {
            XUnmapWindow(dpy, c->window);
            c->mapped = 0;
        }
    }

    tile(dpy, clients, client_count);

    draw_bar(dpy, bar_window, gc);

    Client *to_focus = NULL;

    for (int i = 0; i < client_count; i++) {
        if (clients[i].workspace == current_workspace &&
            clients[i].mapped) {
            to_focus = &clients[i];
            break;
        }
    }

    if (to_focus)
        focus_client(dpy, to_focus);

    XFlush(dpy);
}


void SetupColors(Display *dpy)
{
    Colormap cmap = DefaultColormap(dpy, DefaultScreen(dpy));

    XColor color;

    XParseColor(dpy, cmap, border_color, &color);
    XAllocColor(dpy, cmap, &color);
    focused_color = color.pixel;
    XParseColor(dpy, cmap, bar_color, &color);
    XAllocColor(dpy, cmap, &color);
    bar_color_pixel = color.pixel;

    XParseColor(dpy, cmap, "#444444", &color);
    XAllocColor(dpy, cmap, &color);
    unfocused_color = color.pixel;
}


void get_window_title(Display *dpy, Window w, char *title, size_t size)
{
    Atom net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
    Atom utf8_string = XInternAtom(dpy, "UTF8_STRING", False);

    Atom type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *data = NULL;


    if (XGetWindowProperty(
            dpy,
            w,
            net_wm_name,
            0,
            size / sizeof(long),
            False,
            utf8_string,
            &type,
            &format,
            &nitems,
            &bytes_after,
            &data
        ) == Success)
    {
        if (data && type == utf8_string)
        {
            snprintf(title, size, "%s", data);
            XFree(data);
            return;
        }

        if (data)
            XFree(data);
    }


    // Old style WM_NAME
    XTextProperty prop;

    if (XGetWMName(dpy, w, &prop) && prop.value)
    {
        snprintf(title, size, "%s", prop.value);
        XFree(prop.value);
        return;
    }


    // Last resort
    char *fallback = NULL;

    if (XFetchName(dpy, w, &fallback) && fallback)
    {
        snprintf(title, size, "%s", fallback);
        XFree(fallback);
        return;
    }


    title[0] = '\0';
}


Client *get_focused_client(void)
{
    for (int i = 0; i < client_count; i++) {
        if (clients[i].focused)
            return &clients[i];
    }

    return NULL;
}


int is_dialog(Display *dpy, Window w)
{
    Atom type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom dialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    Atom actual;
    int format;
    unsigned long nitems;
    unsigned long bytes;
    unsigned char *data = NULL;

    if (XGetWindowProperty(
            dpy,
            w,
            type,
            0,
            1,
            False,
            XA_ATOM,
            &actual,
            &format,
            &nitems,
            &bytes,
            &data) == Success)
    {
        if (data) {
            Atom value = *(Atom *)data;
            XFree(data);

            return value == dialog;
        }
    }

    return 0;
}


void move_to_workspace(Display *dpy, Client *c, int target_workspace)
{
    if (!c || !c->mapped)
        return;

    c->workspace = target_workspace;

    if (target_workspace == current_workspace) {
        XMapWindow(dpy, c->window);
        c->mapped = 1;
    } else {
        XUnmapWindow(dpy, c->window);
        c->mapped = 0;
    }

    if (target_workspace != current_workspace) {
        c->focused = false;
    }

    tile(dpy, clients, client_count);
}
