#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>
#include "image.h"
#include "queue.h"

void* create_shared_memory(size_t size) {
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_SHARED | MAP_ANONYMOUS;

  // The remaining parameters to `mmap()` are not important for this use case,
  // but the manpage for `mmap` explains their purpose.
  return mmap(NULL, size, protection, visibility, -1, 0);
}

struct pixel* generate_image(int width, int height)
{
    struct pixel *image;
    image = malloc(width*height*sizeof(struct pixel));
    srand(time(0));
    // we need to fill first row with random colors
    for(int i=0; i<width; i++)
    {
        image[i].red = rand()%255;
        image[i].green = rand()%255;
        image[i].blue = rand()%255;
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
    int width, height, shared_or_queue;
    int packet_size, image_size, pixels_size;
    queue_t* generator_converter;
    char* queue_name = "generator_converter";
    char* ready = "1";
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
        int rv = queue_create(queue_name, (size_t)(20*packet_size));
        if(rv != QUEUE_OK)
        {
            printf("Failed to create queue");
            return 0;
        }
        generator_converter = queue_acquire(queue_name, QUEUE_MASTER);
    }
    else
    {
        int rv = queue_create(queue_name, (size_t)(20*sizeof(ready)));
        if(rv != QUEUE_OK)
        {
            printf("Failed to create queue");
            return 0;
        }
        generator_converter = queue_acquire(queue_name, QUEUE_MASTER);
    }
    while(1)
    {
        struct timespec start = {0}, stop = {0};
        struct image* generated_image;
        struct pixel* just_pixels;
        struct packet* data;        
        clock_gettime(CLOCK_REALTIME, &start);
        printf("dupka 1");
        just_pixels = generate_image(width, height);
        printf("dupka 1");
        generated_image = malloc(image_size);
        (*generated_image).width = width;
        (*generated_image).height = height;
        memcpy((*generated_image).pixel_info, just_pixels, pixels_size);
        free(just_pixels);
        // above we generated random image, now we need to send it
        data = malloc(packet_size);
        memcpy((*data).data, generated_image, image_size);
        free(generated_image);
        (*data).start = start;
        if(shared_or_queue == 0)
        {
            clock_gettime(CLOCK_REALTIME, &stop);
            (*data).stop = stop;
            queue_sync_write(generator_converter, (char*)data, packet_size);
            free(data);
        }
        else
        {
            void* shmem = create_shared_memory(packet_size);
            clock_gettime(CLOCK_REALTIME, &stop);
            (*data).stop = stop;
            memcpy(shmem, data, packet_size);
            free(data);
            queue_sync_write(generator_converter, ready, sizeof(ready));
        }
        printf("Message sent");
    }
    return 0;
}