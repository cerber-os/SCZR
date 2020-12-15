#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "image.h"
#include "queue.h"

int main(int argc, char **argv)
{
    int width, height, shared_or_queue;
    int packet_size, image_size, pixels_size;
    queue_t* generator_converter;
    char* queue_name = "tmp_QUEUE_GEN_CONV";    // from init.c
    if(argc<2)
    {
        printf("Not enough arguments\n");
        return 1;
    }
    else if(argc<4)
    {
        printf("Not enough arguments. We will use default width and height\n");
        shared_or_queue = atoi(argv[1]);
        width = height = 2000;
    }
    else
    {
        shared_or_queue = atoi(argv[1]);
        width = atoi(argv[2]);
        height = atoi(argv[3]);
    }
    pixels_size = width*height*sizeof(struct pixel);
    image_size = 2*sizeof(int) + pixels_size;
    packet_size = 2*sizeof(struct timespec) + image_size;
    // shared_or_queue will be 0 if we want to transmit all data thru queue
    if(shared_or_queue == 0)
    {
        printf("I will try to acquire queue\n");
        generator_converter = queue_acquire(queue_name, QUEUE_SLAVE);
        printf("I have queue\n");
    }
    else
    {
        generator_converter = queue_acquire(queue_name, QUEUE_SLAVE);
        printf("I have queue\n");
    }
    while(1)
    {
        char* buffer;
        printf("I am waiting for message\n");
        int ret = queue_sync_read(generator_converter, &buffer, &packet_size);
        if(ret == 0) {
            // valid message
        }
        printf("I got message\n");
        free(buffer);
    }
    return 0;
}