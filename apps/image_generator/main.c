#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>
#include "image.h"

#include "queue.h"
#include "shared_mem.h"

struct pixel* generate_image(int width, int height)
{
    struct pixel *image;
    image = malloc(width*height*sizeof(struct pixel));
    srand(time(0));
    // we need to fill first row with random colors
    for(int i=0; i<width; i++)
    {
        image[i].red = rand()%255;
        image[i].green = (image[i].red + 20)%255;
        image[i].blue = (image[i].green + 20)%255;
    }
    // we fill the rest of rows
    for(int i=1; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            image[i*width+j].red = image[(i*width+j)%width].red;
            image[i*width+j].green = image[(i*width+j)%width].green;
            image[i*width+j].blue = image[(i*width+j)%width].blue;
        }
    }
    return image;
}

int main(int argc, char **argv)
{
    char memory_name[64] = {0,};
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
    int rv = queue_create(queue_name, (size_t)(50*message_size));	
    if(rv != QUEUE_OK)	
    {	
        printf("Failed to create queue");	
        return 0;	
    }
    queue_ptr = queue_acquire(queue_name, QUEUE_MASTER);
    for(int i = 0; 1; i++)
    {
        struct timespec start = {0}, stop = {0};
        struct image* generated_image;
        struct pixel* just_pixels;
        struct packet* data;        
        clock_gettime(CLOCK_REALTIME, &start);
        just_pixels = generate_image(width, height);
        generated_image = malloc(image_size);
        (*generated_image).width = width;
        (*generated_image).height = height;
        memcpy((*generated_image).pixel_info, just_pixels, pixels_size);
        free(just_pixels);
        // above we generated random image, now we need to send it
        // shared_or_queue will be 0 if we want to transmit all data thru queue
        if(shared_or_queue == 0)
        {
            data = malloc(packet_size);
            memcpy((*data).data, generated_image, image_size);
            free(generated_image);
            (*data).start = start;
            clock_gettime(CLOCK_REALTIME, &stop);
            (*data).stop = stop;
            queue_sync_write(queue_ptr, (char*)data, packet_size);
            free(data);
        }
        else
        {
            snprintf(memory_name, sizeof(memory_name) - 1, "/shmem_gen_%d", i);
            data = (struct packet*) create_shared_memory(memory_name, packet_size);
            if(data == NULL) {
                printf("Failed to create shared memory!\n");
                continue;
            }

            memcpy(data->data, generated_image, image_size);
            free(generated_image);
            data->start = start;
            clock_gettime(CLOCK_REALTIME, &stop);
            data->stop = stop;
            queue_sync_write(queue_ptr, memory_name, strlen(memory_name));
            
            // Release shared memory
            munmap(data, packet_size);
        }
        printf("Message sent\n");
    }
    return 0;
}
