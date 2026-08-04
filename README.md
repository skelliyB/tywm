# tywm — X11 Tiling Window Manager

`tywm` is a lightweight tiling window manager written in C using the Xlib library. It manages X11 windows, provides tiling and floating layouts, keyboard shortcuts, workspaces, a status bar, configurable colors, wallpaper handling, and basic window manipulation.

## Features

* X11-based window management
* Automatic tiling layout
* Floating windows
* Window dragging and resizing with mouse
* Multiple workspaces (1–9)
* Keyboard-driven controls
* Customizable keybindings
* Config file support
* Status bar
* Window borders with focused/unfocused colors
* Wallpaper support using `feh`
* Auto-start applications
* Window title tracking
* Dialog window detection

---

## Requirements

The following packages are required:

### Libraries

* Xlib
* X11 development headers
* XKB extension support

On Debian/Ubuntu:

```bash
sudo apt install libx11-dev libxkbfile-dev
```

You will also need:

```bash
feh
```

for wallpaper support.

---

## Building

Example compilation:

```bash
gcc main.c bar.c wm.c -o tywm \
    -lX11 \
    -lXext
```

Run:

```bash
./tywm
```

Make sure no other window manager is currently running.

---

# Configuration

Configuration is loaded from:

```
~/.config/tywm/tywm.conf
```

If the file does not exist, default settings are used.

Example configuration:

```ini
terminal=kitty
launcher=rofi -show drun

bar_color=#344d5e
bar_height=20
bar_visible=true

background_wallpaper=/path/to/wallpaper.png

border_width=3
border_color=#3e5f9c

autoexec1=nm-applet
autoexec2=picom

float_keybind=f
terminal_keybind=Return
kill_keybind=c
focus_next_keybind=k
unfloat_keybind=v
toggle_bar_keybind=b
launcher_keybind=r
```

---

# Keybindings

## Applications

| Shortcut        | Action                    |
| --------------- | ------------------------- |
| Super + Return  | Open terminal             |
| Alt + R         | Open application launcher |
| Alt + Shift + R | Reload configuration      |

---

## Window Control

| Shortcut | Action                       |
| -------- | ---------------------------- |
| Alt + C  | Kill focused window          |
| Alt + F  | Toggle floating mode         |
| Alt + V  | Return window to tiling mode |
| Alt + K  | Focus next window            |
| Alt + B  | Toggle bar visibility        |

---

## Workspaces

The window manager supports 9 workspaces.

### Switch workspace

```
Alt + 1-9
```

Moves to the selected workspace.

### Move window to workspace

```
Alt + Shift + 1-9
```

Moves the focused window to another workspace.

---

# Mouse Controls

## Move floating windows

```
Alt + Left Click + Drag
```

Moves the selected window.

---

## Resize floating windows

```
Alt + Right Click + Drag
```

Resizes the selected window.

---

# Window Management

## Tiling

Normal windows are automatically arranged horizontally.

Example:

```
+-----------+-----------+-----------+
|           |           |           |
| Window 1  | Window 2  | Window 3  |
|           |           |           |
+-----------+-----------+-----------+
```

The layout ignores floating windows.

---

## Floating Windows

Windows are automatically floated if they are:

* Dialog windows
* Transient windows

Floating windows can be:

* Moved
* Resized
* Raised above tiled windows

---

# Architecture

## Main Components

### X11 Event Loop

The main loop listens for:

* Keyboard events
* Window creation
* Window destruction
* Mouse movement
* Mouse clicks
* Window property changes

Example:

```c
while (running) {
    XNextEvent(dpy, &event);
}
```

---

## Client Management

Windows are stored in a client array:

```c
#define MAX_CLIENTS 64

Client clients[MAX_CLIENTS];
```

Each client tracks:

* Window ID
* Position
* Size
* Workspace
* Floating state
* Focus state
* Window title

---

## Workspace System

The manager supports:

```c
#define MAX_WORKSPACES 9
```

Each window stores:

```c
client.workspace
```

Switching workspaces hides windows from inactive workspaces.

---

# Important Functions

## Window Layout

### `tile()`

Places tiled windows into the current workspace layout.

```c
void tile(Display *dpy, Client *clients, int client_count);
```

---

## Floating Windows

### `float_client()`

Turns a window into floating mode.

```c
void float_client(
    Display *dpy,
    Client *c,
    int x,
    int y,
    unsigned int width,
    unsigned int height
);
```

---

### `unfloat_client()`

Returns a window to the tiled layout.

---

## Focus Management

### `focus_client()`

Handles:

* Active window border color
* X11 input focus
* Window raising

---

## Applications

### `spawn()`

Launches programs:

```c
spawn("kitty");
```

Uses:

```c
fork()
setsid()
exec()
```

---

## Wallpaper

Wallpaper is applied using:

```bash
feh --bg-fill wallpaper.png
```

The function:

```c
spawn_wallpaper()
```

handles this automatically.

---

# Signals

Child processes are automatically cleaned:

```c
signal(SIGCHLD, SIG_IGN);
```

---

# File Structure

Recommended project layout:

```
tywm/
├── main.c
├── wm.c
├── wm.h
├── bar.c
├── bar.h
├── Makefile
└── README.md
```

---

# Known Limitations

* Only supports X11
* No Wayland support
* Single monitor handling
* Limited layout algorithms
* Maximum of 64 clients
* Configuration reload requires rebuilding some runtime state
* No advanced EWMH support

---

# Future Improvements

Possible additions:

* Multiple monitor support
* Master-stack layouts
* Scratchpads
* Better configuration parsing
* IPC support
* Full EWMH compatibility
* Dynamic layouts
* Per-monitor workspaces

---

# License

Choose a license before publishing.

Examples:

* MIT
* GPL-3.0
* BSD-2-Clause

---

# Credits

Built using:

* C
* Xlib
* X11
* XKB
* feh
* rofi
