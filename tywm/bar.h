#ifndef BAR_H
#define BAR_H

#include <X11/Xlib.h>

void draw_bar(Display *dpy, Window bar, GC gc);
void show_bar(Display *dpy);
void hide_bar(Display *dpy);

#endif