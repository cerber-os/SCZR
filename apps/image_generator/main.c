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
        image[i].green = (image[i].red + 20) % 255;
        image[i].blue = (image[i].green + 20)  %255;
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
    char memory_name[128] = {0,};
    int width, height, shared_or_queue;
    size_t packet_size, image_size, pixels_size, message_size;

    printf("[i] image_gen: started\n");

    if(argc<2)
    {
        printf("[-] image_gen: Not enough arguments\n");
        return 1;
    } else if(argc < 4) {
        printf("[i] image_gen: Not enough arguments. We will use default width and height\n");
        shared_or_queue = atoi(argv[1]);
        width = height = 2000;
    } else {
        shared_or_queue = atoi(argv[1]);
        width = atoi(argv[2]);
        height = atoi(argv[3]);
    }

    pixels_size = width*height*sizeof(struct pixel);
    image_size = 2*sizeof(int) + pixels_size;
    packet_size = 2*sizeof(struct timespec) + image_size;
    message_size = 128;

    queue_t* queue_ptr = queue_acquire("tmp_QUEUE_GEN_CONV", QUEUE_MASTER);
    if(queue_ptr == NULL) {
        printf("[-] image_gen: failed to acquire queue `tmp_QUEUE_GEN_CONV`\n");
        return 1;
    }

    for(int i = 0; 1; i++)
    {
        struct timespec start = {0}, stop = {0};
        struct image* generated_image;
        struct pixel* just_pixels;
        struct packet* data;

        clock_gettime(CLOCK_REALTIME, &start);
        just_pixels = generate_image(width, height);
        generated_image = malloc(image_size);
        generated_image->width = width;
        generated_image->height = height;
        memcpy(generated_image->pixel_info, just_pixels, pixels_size);
        free(just_pixels);

        // above we generated random image, now we need to send it
        // shared_or_queue will be 0 if we want to transmit all data thru queue
        if(shared_or_queue == 0)
        {
            data = malloc(packet_size);
            memcpy(data->data, generated_image, image_size);
            free(generated_image);
            data->start = start;
            clock_gettime(CLOCK_REALTIME, &stop);
            data->stop = stop;
            queue_sync_write(queue_ptr, (char*)data, packet_size);
            free(data);
        } else {
            snprintf(memory_name, sizeof(memory_name) - 1, "/shmem_gen_%d", i);
            data = (struct packet*) create_shared_memory(memory_name, packet_size);
            if(data == NULL) {
                printf("[-] image_gen: failed to create shared memory!\n");
                continue;
            }

            memcpy(data->data, generated_image, image_size);
            free(generated_image);
            data->start = start;
            clock_gettime(CLOCK_REALTIME, &stop);
            data->stop = stop;
            queue_sync_write(queue_ptr, memory_name, message_size);
            
            // Release shared memory
            munmap(data, packet_size);
        }

        printf("[i] image_gen: message sent\n");

        while(queue_get_pending(queue_ptr) > 0)
            usleep(50 * 1000);
    }
    return 0;
}
