/*
 * Implementation of process visualizing other apps performance
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "graphics.h"
#include "image.h"
#include "queue.h"

enum hyp_mode {
    MODE_TRANSMITTER = 0,
    MODE_RECEIVER = 1,
};
int mode;

char* window_title[] = {"TRANSMITTER", "RECEIVER"};
hyp_window* win;

queue_t* q_hyp_gen;


char* messages [][3] = {
{"Generator stats (diff) = 1","Compressor stats (diff) = 4","Speed (B) = 7200bytes/sec (0.2 IMG/s)"},
};


char* messages2 [][5] = {
{"Validator stats (diff) = 5369","Compressor stats (diff) = 4203","Latency = 2","Speed (B) = 7200bytes/sec (0.2 IMG/s)", "result: OK"},
};

int stat_gen, stats_comp, stats_val, stats_comp_2, stats_latency, stats_result;

void window_task() {
    char line[128];

    // Title at the top
    draw_string(win, 10, 10, window_title[mode]);

    // Image section
    int image_width = 200;
    struct pixel* img = malloc(200 * 80 * sizeof(struct pixel));
    draw_image(win, 5, 20, image_width, 80, img);
    free(img);


    // Stats section
    const int stats_start = 25;
    draw_string(win, image_width + 10, stats_start, "Statistics:");
    draw_string(win, image_width + 10, stats_start + 11, "IMG format = 200 x 80 x 24bit");
    draw_string(win, image_width + 10, stats_start + 22, "Interface baudrate = 76800/s");

    if (mode == MODE_TRANSMITTER) {
        snprintf(line, sizeof(line) - 1, "Generator stats (diff) = %d", stat_gen);
        draw_string(win, image_width + 10, stats_start + 33,line);
        snprintf(line, sizeof(line) - 1, "Compressor stats (diff) = %d", stats_comp);
        draw_string(win, image_width + 10, stats_start + 44, line);
        draw_string(win, image_width + 10, stats_start + 55, "Speed (B) = 7200bytes/sec (0.2 IMG/s)");
    }else {
        snprintf(line, sizeof(line) - 1, "Validator stats (diff) = %d", stats_val);
        draw_string(win, image_width + 10, stats_start + 33, line);
        snprintf(line, sizeof(line) - 1, "Compressor stats (diff) = %d", stats_comp_2);
        draw_string(win, image_width + 10, stats_start + 44, line);
        snprintf(line, sizeof(line) - 1, "Latency stats (diff) = %d", stats_latency);
        draw_string(win, image_width + 10, stats_start + 55, line);
        draw_string(win, image_width + 10, stats_start + 66, "Speed (B) = 7200bytes/sec (0.2 IMG/s)");
        if(!stats_result)
            draw_string(win, image_width + 10, stats_start + 77, "Result: Ok");
        else
            draw_string(win, image_width + 10, stats_start + 77, "Result: FAIL");
    }
}

int stats_task(queue_t* queue_client) {
    char* buffer;
    size_t size;

    int ret = queue_async_read(queue_client, (void**)&buffer, &size);
    if(ret == 0) {
        printf("[i] hypervisor: Got a new message - %zu\n", size);
        // TODO
        free(buffer);
    }
    return 0;
}

int main(int argc, char** argv) {
    srand(0);
    fprintf(stderr, "[i] hypervisor: started\n");
    
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "mode=TRANSMITTER")) 
            mode = MODE_TRANSMITTER;
        else if(!strcmp(argv[i], "mode=RECEIVER"))
            mode = MODE_RECEIVER;
        else {
            fprintf(stderr, "[-] hypervisor: Invalid option was provided '%s'\n", argv[i]);
            exit(1);
        }
    }

    win = init_window();
    if(win == NULL) {
        fprintf(stderr, "[-] hypervisor: Failed to init display\n");
        exit(1);       
    }

    queue_t* queue_client;
    if(mode == MODE_TRANSMITTER) {
        queue_client = queue_acquire("tmp_QUEUE_HYP_CONV", QUEUE_SLAVE);
        if(queue_client == NULL) {
            printf("[-] hypervisor: Failed to acquire queue\n");
            return 1;
        }
    } else { // RECEIVER
        queue_client = queue_acquire("tmp_QUEUE_HYP_VAL", QUEUE_SLAVE);
        if(queue_client == NULL) {
            printf("[-] hypervisor: Failed to acquire queue\n");
            return 1;
        }
    }   


    window_task();

    while(1) {
        clear_screen(win);
        window_task();
        XSync(win->display, False);
        stats_task(queue_client);

        usleep(1000 * 1000);     // 099s
    }

    close_window(win);
}
