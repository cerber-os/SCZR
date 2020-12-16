#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>

#include "image.h"
#include "queue.h"
#include "shared_mem.h"


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
            if(image[i*width+j].blue != image[(i*width+j)%width].blue)
                rv= 1;
        }
    }
    return rv;
}

int main(int argc, char **argv)
{
    int width, height, shared_or_queue;
    size_t packet_size, image_size, pixels_size, message_size, new_packet_size, new_new_new_packet_size, new_new_packet_size;
    queue_t* queue_ptr;
    char* queue_name = "tmp_QUEUE_VAL_CONV";    // from init.c
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
    new_packet_size = 2*sizeof(struct timespec) + packet_size;
    new_new_packet_size = 2*sizeof(struct timespec) + new_packet_size;
    new_new_new_packet_size = 2*sizeof(struct timespec) + new_packet_size;
    message_size = sizeof(char*);
    queue_ptr = queue_acquire(queue_name, QUEUE_SLAVE);
    queue_t* queue_ptr_2;
    char* queue_name_2 = "tmp_QUEUE_HYP_VAL";    // from init.c
    queue_ptr_2 = queue_acquire(queue_name_2, QUEUE_SLAVE);
    while(1)
    {
        char* buffer;
        struct timespec start = {0}, stop = {0};
        clock_gettime(CLOCK_REALTIME, &start);

	if(shared_or_queue == 0)
        {
            int ret = queue_sync_read(queue_ptr, &buffer, &new_new_packet_size);
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
            char* name = buffer;
            buffer = get_shared_memory(name, packet_size);
            if(buffer == NULL) {
                printf("Failed to get shared memory %s\n", name);
                delete_shared_memory(name); // Remove anyway
                continue;    
            }
            delete_shared_memory(name);
        }
        void *copy_buffer;
        copy_buffer = malloc(pixels_size);
        memcpy(copy_buffer, buffer+(new_new_packet_size-pixels_size), pixels_size);
        int rv = validate((struct pixel*)copy_buffer, width, height);
        printf("RV value %d\n", rv);
        struct packet* for_hypervisor;
        for_hypervisor = malloc(new_new_new_packet_size);
        memcpy((*for_hypervisor).data, buffer, new_packet_size);
        (*for_hypervisor).start = start;
        clock_gettime(CLOCK_REALTIME, &stop);
        (*for_hypervisor).stop = stop;
        if(shared_or_queue == 0)
        {
            queue_sync_write(queue_ptr_2, (char*)for_hypervisor, new_new_new_packet_size);
            free(for_hypervisor);
        }
        else
        {
            void *data = (struct packet*) create_shared_memory(new_new_new_packet_size);
            memcpy(data, for_hypervisor, new_new_new_packet_size);
            queue_sync_write(queue_ptr_2, (char*)&data, sizeof(char*));
        }
        if(shared_or_queue == 0)
        {
            free(buffer);
        }
        else
        {
            munmap(buffer, new_new_packet_size);
        }
    }
    return 0;
}
