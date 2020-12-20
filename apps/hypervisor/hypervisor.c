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
#include "misc.h"

#define TIME_MODULO         1000000000UL 

enum hyp_mode {
    MODE_TRANSMITTER = 0,
    MODE_RECEIVER = 1,
};
int mode;


char* window_title[] = {"TRANSMITTER", "RECEIVER"};
hyp_window* win;

int image_width, image_height;

void dump_stats(struct packet* packet) {
    static long i = 0;
    char* titles[] = {
        "No.",
        "GEN_START", "GEN_STOP", "GEN_DIFF",
        "CONV_T_START", "CONV_T_STOP", "CONV_T_DIFF",
        "CONV_R_START", "CONV_R_STOP", "CONV_R_DIFF",
        "VAL_START", "VAL_STOP", "VAL_DIFF",
        "QUEUE_GEN_CONV", "QUEUE_CONV_CONV", "QUEUE_CONV_VAL",
        "TOTAL_DIFF", "VALID_RESULT"
    };

    if(i == 0) {
        printf("[CSV] ");
        for(int i = 0; i < sizeof(titles) / sizeof(titles[0]); i++)
            printf("%s;", titles[i]);
        printf("\n");
    }

    printf("[CSV] %ld;", i++);
    printf("%ld;%ld;%ld;", 
            time_start_stage_us(packet, STAGE_T_GENERATOR),
            time_stop_stage_us(packet, STAGE_T_GENERATOR),
            time_spent_in_one_stage_us(packet, STAGE_T_GENERATOR));
    printf("%ld;%ld;%ld;", 
            time_start_stage_us(packet, STAGE_T_CONV),
            time_stop_stage_us(packet, STAGE_T_CONV),
            time_spent_in_one_stage_us(packet, STAGE_T_CONV));
    printf("%ld;%ld;%ld;", 
            time_start_stage_us(packet, STAGE_R_CONV),
            time_stop_stage_us(packet, STAGE_R_CONV),
            time_spent_in_one_stage_us(packet, STAGE_R_CONV));
    printf("%ld;%ld;%ld;", 
            time_start_stage_us(packet, STAGE_R_VALIDATOR),
            time_stop_stage_us(packet, STAGE_R_VALIDATOR),
            time_spent_in_one_stage_us(packet, STAGE_R_VALIDATOR));
    printf("%ld;%ld;%ld;",
            time_between_stages_us(packet, STAGE_T_GENERATOR, STAGE_T_CONV),
            time_between_stages_us(packet, STAGE_T_CONV, STAGE_R_CONV),
            time_between_stages_us(packet, STAGE_R_CONV, STAGE_R_VALIDATOR));
    printf("%ld;",
            time_between_stages_us(packet, STAGE_T_GENERATOR, STAGE_R_VALIDATOR));
    printf("%d\n", packet->valid_result);

}

void window_task(struct packet* packet) {
    char line[256];

    // Title at the top
    draw_string(win, 10, 10, window_title[mode]);

    // Image section with scale 1
    const int scale = 4;
    struct pixel* img = (struct pixel*)packet->data;
    draw_image(win, 5, 20, image_width, image_height, img, 1);


    // Stats section
    int stats_start = 25;
    int scaled_image_width = image_width / scale;
    draw_string(win, scaled_image_width + 10, stats_start, "Statistics:"); stats_start += 11;
    snprintf(line, sizeof(line) - 1, "IMG format = %d x %d x 24bit", scaled_image_width, image_height);
    draw_string(win, scaled_image_width + 10, stats_start, line); stats_start += 11;
    draw_string(win, scaled_image_width + 10, stats_start, "Interface baudrate = xxxxx/s"); stats_start += 11;

    if (mode == MODE_TRANSMITTER) {
        snprintf(line, sizeof(line) - 1, "Generator:      start=%ld end=%ld diff=%ld",
                time_start_stage_us(packet, STAGE_T_GENERATOR) % TIME_MODULO,
                time_stop_stage_us(packet, STAGE_T_GENERATOR) % TIME_MODULO,
                time_spent_in_one_stage_us(packet, STAGE_T_GENERATOR));
        draw_string(win, scaled_image_width + 10, stats_start,line); stats_start += 11;

        snprintf(line, sizeof(line) - 1, "Queue gen_conv: diff=%ld",
                time_between_stages_us(packet, STAGE_T_GENERATOR, STAGE_T_CONV));
        draw_string(win, scaled_image_width + 10, stats_start, line); stats_start += 11;

        snprintf(line, sizeof(line) - 1, "Converter:      start=%ld end=%ld diff=%ld",
                time_start_stage_us(packet, STAGE_T_CONV) % TIME_MODULO,
                time_stop_stage_us(packet, STAGE_T_CONV) % TIME_MODULO,
                time_spent_in_one_stage_us(packet, STAGE_T_CONV));
        draw_string(win, scaled_image_width + 10, stats_start, line); stats_start += 11;

    } else {
        snprintf(line, sizeof(line) - 1, "Arduino pipe:   diff=%ld",
                time_between_stages_us(packet, STAGE_T_CONV, STAGE_R_CONV));
        draw_string(win, scaled_image_width + 10, stats_start,line); stats_start += 11;

        snprintf(line, sizeof(line) - 1, "Converter:      start=%ld end=%ld diff=%ld",
                time_start_stage_us(packet, STAGE_R_CONV) % TIME_MODULO,
                time_stop_stage_us(packet, STAGE_R_CONV) % TIME_MODULO,
                time_spent_in_one_stage_us(packet, STAGE_R_CONV));
        draw_string(win, scaled_image_width + 10, stats_start,line); stats_start += 11;

        snprintf(line, sizeof(line) - 1, "Queue conv_val: diff=%ld",
                time_between_stages_us(packet, STAGE_R_CONV, STAGE_R_VALIDATOR));
        draw_string(win, scaled_image_width + 10, stats_start,line); stats_start += 11;

        snprintf(line, sizeof(line) - 1, "Validator:      start=%ld end=%ld diff=%ld",
                time_start_stage_us(packet, STAGE_R_VALIDATOR) % TIME_MODULO,
                time_stop_stage_us(packet, STAGE_R_VALIDATOR) % TIME_MODULO,
                time_spent_in_one_stage_us(packet, STAGE_R_VALIDATOR));
        draw_string(win, scaled_image_width + 10, stats_start,line); stats_start += 11;

        snprintf(line, sizeof(line) - 1, "Total:          start=%ld end=%ld diff=%ld",
                time_start_stage_us(packet, STAGE_T_GENERATOR) % TIME_MODULO,
                time_stop_stage_us(packet, STAGE_R_VALIDATOR) % TIME_MODULO,
                time_between_stages_us(packet, STAGE_T_GENERATOR, STAGE_R_VALIDATOR));
        draw_string(win, scaled_image_width + 10, stats_start,line); stats_start += 11;

        snprintf(line, sizeof(line) - 1, "Result:         %s (%d diff)",
                (packet->valid_result == 0) ? "OK": "FAIL",
                packet->valid_result);
        draw_string(win, scaled_image_width + 10, stats_start, line); stats_start += 11;
    }
}

struct packet* stats_task(queue_t* queue_client) {
    char* buffer;
    size_t size;

    int ret = queue_sync_read(queue_client, (void**)&buffer, &size);
    if(ret == 0) {
        // printf("[i] hypervisor: Got a new message - %zu\n", size);
        return (struct packet*) buffer;
    }
    return NULL;
}

int main(int argc, char** argv) {
    fprintf(stderr, "[i] hypervisor: started\n");
    
    if(argc < 4) {
        printf("[-] hypervisor: missing arguments\n");
        return 1;
    } else {
        if(strstr(argv[1], "mode=TRANSMITTER"))
            mode = MODE_TRANSMITTER;
        else
            mode = MODE_RECEIVER;
        image_width = atoi(argv[2]);
        image_height = atoi(argv[3]);
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

    while(1) {
        struct packet* packet = stats_task(queue_client);
        if(packet == NULL)
            continue;

        dump_stats(packet);

        clear_screen(win);
        window_task(packet);
        XSync(win->display, False);

        free(packet);
    }

    close_window(win);
}
