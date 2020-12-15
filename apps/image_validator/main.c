#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>
#include "image.h"
#include "queue.h"


int validate(struct pixel* image, int width, int height)
{
    int rv = 0;
    for(int i=1; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            if(image[i*width+j].red != image[(i*width+j)%width].red)
                rv = 1;
            if(image[i*width+j].green != image[(i*width+j)%width].green)
                rv = 1;
            if(image[i*width+j].blue = image[(i*width+j)%width].blue)
                rv= 1;
        }
    }
    return rv;
}


int main(int argc, char **argv)
{
    int width, height, shared_or_queue;
    int packet_size, image_size, pixels_size, message_size;
    queue_t* queue_ptr;
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
    message_size = sizeof(char*);
    queue_ptr = queue_acquire(queue_name, QUEUE_SLAVE);
    int i = 1;
    while(1)
    {
        void* buffer;
        printf("%d\n", i);
        i++;
        if(shared_or_queue == 0)
        {
            int ret = queue_sync_read(queue_ptr, &buffer, &packet_size);
            if(ret != 0) {
                continue;
            }
        }
        else
        {
            int ret = queue_sync_read(queue_ptr, &buffer, &message_size);
            if(ret != 0) {
                continue;
            }
        }
        void *copy_buffer;
        copy_buffer = malloc(pixels_size);
        memcpy(copy_buffer, buffer+2*(sizeof(struct timespec)+sizeof(int)), pixels_size);
        int rv = validate((struct pixel*)copy_buffer, width, height);
        printf("RV value %d\n", rv);
        if(shared_or_queue == 0)
        {
            free(buffer);
        }
        else
        {
            munmap(buffer, packet_size);
        }
    }
    return 0;
}