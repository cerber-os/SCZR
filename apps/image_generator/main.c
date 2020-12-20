#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "image.h"
#include "queue.h"
#include "misc.h"

double func(double x)
{
    return exp(x)+exp(-x);
}

double min(double a, double b)
{
    if(a<b)
        return a;
    else
        return b;
}

double max(double a, double b)
{
    if(a>b)
        return a;
    else
        return b;
}

double generator(double a, double b) // generate random numbers in given range
{
    return a + ((double)rand()/((double)(RAND_MAX)+1) * (b-a));
}

// we try to calculate integral
int monte_carlo(int number_of_iterations)
{
    double x, y, min_x, max_x, tmp, func_max, good;
    good = 0;
    tmp = rand()%5 + 1;
    min_x = generator(0, tmp);
    max_x = generator(min_x + 6, min_x + 10);
    func_max = max(func(min_x), func(max_x));
    for(int i=0; i<number_of_iterations; i++)
    {
        x = generator(min_x, max_x);
        y = generator(0, func_max);
        if(y <= func(x))
            good++;
    }
    double area = (good/(double)number_of_iterations)*func_max*(max_x-min_x);
    int pixel_value;
    pixel_value = (int)area;
    pixel_value = pixel_value%255;
    return pixel_value;
}

void generate_image(struct packet* packet, int width, int height, int number_of_iterations)
{
    struct pixel *image = (struct pixel*) packet->data;

    // we need to fill first row with random colors
    for(int i=0; i<width; i++)
    {
        image[i].red = monte_carlo(number_of_iterations);
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
        memcpy(packet->magic, &PACKET_MAGIC_VALUE, sizeof(PACKET_MAGIC_VALUE));

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
