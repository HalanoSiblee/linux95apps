#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define WIN_W 400
#define WIN_H 100
#define MAX_CMDS 10000

char buf[256] = "", *cmds[MAX_CMDS];
int len = 0, cmd_count = 0;

void scan_path() {
    char *env = getenv("PATH");
    if (!env) return;
    char *path = strdup(env), *dir = strtok(path, ":");
    while (dir && cmd_count < MAX_CMDS) {
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *de;
            while ((de = readdir(d)) && cmd_count < MAX_CMDS) {
                if (de->d_name[0] != '.') cmds[cmd_count++] = strdup(de->d_name);
            }
            closedir(d);
        }
        dir = strtok(NULL, ":");
    }
    free(path);
}

void redraw(Display *d, Window w, GC gc) {
    XClearWindow(d, w);
    XSetForeground(d, gc, 0x000000);
    XDrawString(d, w, gc, 20, 30, "Run Command:", 12);
    XSetForeground(d, gc, 0xffffff);
    XFillRectangle(d, w, gc, 20, 45, 360, 25);
    XSetForeground(d, gc, 0x000000);
    XDrawRectangle(d, w, gc, 20, 45, 360, 25);
    if (len > 0) XDrawString(d, w, gc, 30, 62, buf, len);
    XFlush(d);
}

int main() {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return 1;
    int scr = DefaultScreen(dpy);
    scan_path();

    Atom prop = XInternAtom(dpy, "_NET_WORKAREA", True);
    unsigned long *workarea = NULL;
    Atom actual_type; int actual_fmt; unsigned long nitems, after;
    int x = 10, y = DisplayHeight(dpy, scr) - WIN_H - 10;

    if (prop != None && XGetWindowProperty(dpy, RootWindow(dpy, scr), prop, 0, 4, False, AnyPropertyType,
                                           &actual_type, &actual_fmt, &nitems, &after, (unsigned char**)&workarea) == Success) {
        if (workarea) {
            y = workarea[3] - WIN_H - 5;
            XFree(workarea);
        }
    }

    Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, scr), x, y, WIN_W, WIN_H, 1, 0, 0xd4d0c8);
    
    Atom wmDel = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wmDel, 1);
    XSizeHints *h = XAllocSizeHints();
    h->flags = PMinSize | PMaxSize | PPosition;
    h->min_width = h->max_width = WIN_W; h->min_height = h->max_height = WIN_H;
    XSetWMNormalHints(dpy, win, h);
    XStoreName(dpy, win, "Run");
    XSelectInput(dpy, win, ExposureMask | KeyPressMask);
    XMapWindow(dpy, win);
    GC gc = XCreateGC(dpy, win, 0, NULL);

    Atom CLIP = XInternAtom(dpy, "CLIPBOARD", False), UTF8 = XInternAtom(dpy, "UTF8_STRING", False);

    while (1) {
        XEvent ev; XNextEvent(dpy, &ev);
        if (ev.type == ClientMessage && ev.xclient.data.l[0] == wmDel) break;
        if (ev.type == Expose) redraw(dpy, win, gc);
        if (ev.type == KeyPress) {
            KeySym k; char kbuf[32];
            int n = XLookupString(&ev.xkey, kbuf, sizeof(kbuf), &k, NULL);
            int ctrl = ev.xkey.state & ControlMask;

            if (k == XK_Return) {
                if (len > 0) { char c[300]; snprintf(c, 300, "%s &", buf); system(c); }
                break;
            }
            if (k == XK_Escape) break;
            if (k == XK_BackSpace && len > 0) buf[--len] = '\0';
            else if (ctrl && k == XK_a) { len = 0; buf[0] = '\0'; }
            else if (ctrl && k == XK_v) XConvertSelection(dpy, CLIP, UTF8, CLIP, win, CurrentTime);
            else if (k == XK_Tab && len > 0) { 
                for (int i = 0; i < cmd_count; i++) {
                    if (strncmp(buf, cmds[i], len) == 0) {
                        strncpy(buf, cmds[i], 255);
                        len = strlen(buf);
                        break;
                    }
                }
            } else if (n > 0 && !ctrl && len + n < 250) {
                memcpy(buf + len, kbuf, n);
                len += n; buf[len] = '\0';
            }
            redraw(dpy, win, gc);
        }
        if (ev.type == SelectionNotify) {
            unsigned char *data = NULL;
            XGetWindowProperty(dpy, win, CLIP, 0, 64, False, AnyPropertyType, &actual_type, &actual_fmt, &nitems, &after, &data);
            if (data) { strncat(buf, (char*)data, 250 - len); len = strlen(buf); XFree(data); redraw(dpy, win, gc); }
        }
    }
    XCloseDisplay(dpy);
    return 0;
}
