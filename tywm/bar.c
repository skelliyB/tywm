#include "wm.h"
#include <stdio.h>
#include <string.h>

void draw_bar(Display *dpy, Window bar, GC gc);
void hide_bar(Display *dpy);
void show_bar(Display *dpy);


void draw_bar(Display *dpy, Window bar_window, GC gc)
{

    if (!bar_enabled) {
        XUnmapWindow(dpy, bar_window);
        return;
    }

    XMapRaised(dpy, bar_window);

    XClearWindow(dpy, bar_window);

    char *text = "TyWM";
    int text_len = strlen(text);
    

    XDrawString(
        dpy,
        bar_window,
        gc,
        10, BAR_HEIGHT / 2 + 5,
        text,
        text_len
    );

    XDrawString(
        dpy,
        bar_window,
        gc,
        200, BAR_HEIGHT / 2 + 5,
        "workspace: ",
        strlen("workspace: ")
    );

    XDrawString(
        dpy,
        bar_window,
        gc,
        400, BAR_HEIGHT / 2 + 5,
        "focused window: ",
        strlen("focused window: ")
    );
    Client *focused_client = get_focused_client();

    if (focused_client)
    {
        XDrawString(
            dpy,
            bar_window,
            gc,
            520,
            BAR_HEIGHT / 2 + 5,
            focused_client->title,
            strlen(focused_client->title)
        );
    }
    char workspace_str[10];
    snprintf(workspace_str, sizeof(workspace_str), "%d", current_workspace + 1);
    XDrawString(
        dpy,
        bar_window,
        gc,
        280, BAR_HEIGHT / 2 + 5,
        workspace_str,
        strlen(workspace_str)
    );
}

void hide_bar(Display *dpy)
{
    if (!bar_enabled)
        return;

    XUnmapWindow(dpy, bar_window);

    bar_enabled = false;

    tile(dpy, clients, client_count);
    XFlush(dpy);
}


void show_bar(Display *dpy)
{
    if (bar_enabled)
        return;

    XMapRaised(dpy, bar_window);

    bar_enabled = true;

    tile(dpy, clients, client_count);
    XFlush(dpy);
}
