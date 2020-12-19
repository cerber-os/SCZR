#include <X11/Xlib.h>

#include "image.h"

typedef struct hyp_window {
    Display* display;
    int screen_num;
    Window win;
    Visual* visual;
    GC gc;
} hyp_window;


hyp_window* init_window(void);
void close_window(hyp_window* hwnd);
int draw_string(hyp_window* hwnd, int x, int y, char* s);
int is_screen_exposed(hyp_window* hwnd);
void draw_image(hyp_window* hwnd, int x, int y, int width, int height, struct pixel* buffer);
void clear_screen(hyp_window* hwnd);
