#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>
#include "image.h"

#include "queue.h"
#include "shared_mem.h"
#include "misc.h"

// we try to calculate x^2 integral
int monte_carlo(int max_x, int number_of_iterations)
{
    int x,y,good,bad;
    good = bad = 0;
    for(int i=0; i<number_of_iterations; i++)
    {
        x = rand()%max_x;
        y = rand();
        if(y <= (x^2))
            good++;
    }
    return (int)((good/number_of_iterations)*(max_x^2)*max_x);
}

void generate_image(struct packet* packet, int width, int height, int number_of_iterations)
{
    struct pixel *image = (struct pixel*) packet->data;

    // we need to fill first row with random colors
    for(int i=0; i<width; i++)
    {
        image[i].red = monte_carlo(rand()%20+1,number_of_iterations)%255;
        image[i].green = (image[i].red + 20) % 255;
        image[i].blue = (image[i].green + 20) % 255;
    }

    // we fill the rest of rows
    for(int i=1; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            image[i*width+j].red = image[(j+i)%width].red;
            image[i*width+j].green = image[(j+i)%width].green;
            image[i*width+j].blue = image[(j+i)%width].blue;
        }
    }
}

int main(int argc, char **argv)
{
    int width, height, number_of_iterations;
    printf("[i] image_gen: started\n");
    srand(time(0));

    if(argc < 4) {
        printf("[i] image_gen: using default width and height (2000x2000)\n");
        width = height = 2000;
    } else {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        number_of_iterations = atoi(argv[3]);
    }

    const size_t packet_size = IMAGE_SIZE(width, height) + sizeof(struct packet);
    struct packet* packet = malloc(packet_size);
    if(packet == NULL) {
        printf("[i] image_gen: failed to allocate packet buffer\n");
        return 1;
    }

    queue_t* queue_to_conv = queue_acquire("tmp_QUEUE_GEN_CONV", QUEUE_MASTER);
    if(queue_to_conv == NULL) {
        printf("[-] image_gen: failed to acquire queue `tmp_QUEUE_GEN_CONV`\n");
        return 1;
    }

    for(int i = 0; 1; i++)
    {
        memset(packet, 0, packet_size);

        set_start_time_now(packet, STAGE_T_GENERATOR);

        generate_image(packet, width, height, number_of_iterations);

        set_stop_time_now(packet, STAGE_T_GENERATOR);

        queue_sync_write(queue_to_conv, packet, packet_size);
        printf("[i] image_gen: message sent\n");

        // Wait, till converter receives an image
        while(queue_get_pending(queue_to_conv) > 0)
            usleep(50 * 1000);
    }

    free(packet);
    return 0;
}
