/*
 * Implementation of process visualizing other apps performance
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "graphics.h"

#include "queue.h"

enum hyp_mode {
    MODE_TRANSMITTER = 0,
    MODE_RECEIVER = 1,
};
int mode;

char* window_title[] = {"TRANSMITTER", "RECEIVER"};
hyp_window* win;

queue_t* q_hyp_gen;

void window_task() {
    // Title at the top
    draw_string(win, 10, 10, window_title[mode]);

    // Image section
    int image_width = 200;
    draw_image(win, 5, 20, image_width, 0, 0);

    // Stats section
    const int stats_start = 25;
    draw_string(win, image_width + 10, stats_start, "Statistics:");
    draw_string(win, image_width + 10, stats_start + 11, "Generator idle = 20%");
    draw_string(win, image_width + 10, stats_start + 22, "Compressor idle = 0%");
    draw_string(win, image_width + 10, stats_start + 33, "Overall speed = 15 IMG/min");
}

int stats_task() {
    return 0;
}

int main(int argc, char** argv) {
    fprintf(stderr, "Hypervisor started\n");
    
    q_hyp_gen = queue_acquire("tmp_QUEUE_HYP_GEN", QUEUE_MASTER);
    if(q_hyp_gen == NULL) {
        fprintf(stderr, "Failed to acquire queue\n");
        exit(1);
    }

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "mode=TRANSMITTER")) 
            mode = MODE_TRANSMITTER;
        else if(!strcmp(argv[i], "mode=RECEIVER"))
            mode = MODE_RECEIVER;
        else {
            fprintf(stderr, "Invalid option was provided '%s'\n", argv[i]);
            exit(1);
        }
    }

    win = init_window();
    if(win == NULL) {
        fprintf(stderr, "Failed to init display\n");
        exit(1);       
    }

    window_task();

    while(1) {
        window_task();
        window_task();

        usleep(500 * 1000);     // 0.5 s
    }

    close_window(win);
}
