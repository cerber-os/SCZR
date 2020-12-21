/*
 * Basic graphic library using plane X.org protocol
 */
#include <X11/Xlib.h>

#include "image.h"

// Structure representing hypervisor window - contains all necessary pointer/descriptors
typedef struct hyp_window {
    Display* display;
    int screen_num;
    Window win;
    Visual* visual;
    GC gc;
} hyp_window;

/*
 * Functions responsible for creating and destroying hypervisor window
 */
hyp_window* init_window(void);
void close_window(hyp_window* hwnd);

/*
 * Set of functions used for operating on with content
 */
int draw_string(hyp_window* hwnd, int x, int y, char* s);
void draw_image(hyp_window* hwnd, int x, int y, int width, int height, struct pixel* buffer, int scale);
int is_screen_exposed(hyp_window* hwnd);
void clear_screen(hyp_window* hwnd);
