#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "graphics.h"



hyp_window* init_window(void) {
    struct hyp_window* hwnd = malloc(sizeof(struct hyp_window));
    if(hwnd == NULL)
        return NULL;

    Display *display = XOpenDisplay(NULL);
    if(display == NULL) {
        fprintf(stderr, "Failed to get DISPLAY\n");
        return NULL;
    }
    
	int screen_num = DefaultScreen(display);
	Window root = RootWindow(display, screen_num);
    Visual *visual=DefaultVisual(display, 0);
    Window win = XCreateSimpleWindow(display, root,
                                     0, 0, 640, 480,        // X, Y, W, H
                                     0,                     // border width
                                     BlackPixel(display,screen_num),    // border color
                                     BlackPixel(display,screen_num));   // background color
    XMapWindow(display, win);
	XSelectInput(display, win, ExposureMask | KeyPressMask);
    GC gc = XCreateGC(display, win, 0, NULL);

    XSetForeground(display,gc,WhitePixel(display, screen_num));

    hwnd->display = display;
    hwnd->screen_num = screen_num;
    hwnd->win = win;
    hwnd->visual = visual;
    hwnd->gc = gc;
    return hwnd;
}


void close_window(hyp_window* hwnd) {
    XCloseDisplay(hwnd->display);
    free(hwnd);
}

int draw_string(hyp_window* hwnd, int x, int y, char* s) {
    return XDrawString(hwnd->display, hwnd->win, hwnd->gc, x, y, s, strlen(s));
} 

int is_screen_exposed(hyp_window* hwnd) {
    XEvent event;
    int ret = XCheckMaskEvent(hwnd->display, ExposureMask, &event);
    if(ret)
        return 1;
    return 0;
}

void draw_image(hyp_window* hwnd, int x, int y, int width, int height, char* buffer) {
    return;
    // TODO: Convery from 24-bits
    XImage* image = XCreateImage(hwnd->display, hwnd->visual, 
                DefaultDepth(hwnd->display, DefaultScreen(hwnd->display)), 
                ZPixmap, 0, buffer, width, height, 32, 0);
    XPutImage(hwnd->display, hwnd->win,
                DefaultGC(hwnd->display, hwnd->screen_num),
                image,
                0, 0,
                x, y,
                width, height);
}