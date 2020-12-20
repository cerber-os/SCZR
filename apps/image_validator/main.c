#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>

#include "image.h"
#include "queue.h"
#include "misc.h"


// TODO:
int validate(struct packet* packet, int width, int height)
{
    struct pixel* image = (struct pixel*) packet->data;
    int rv = 0;
    for(int i=1; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            if(image[i*width+j].red != image[(j+i)%width].red)
                rv += 1;
            if(image[i*width+j].green != (image[(j+i)%width].red + 20) % 255)
                rv += 1;
            if(image[i*width+j].blue != (image[(j+i)%width].green + 20) % 255)
                rv += 1;
        }
    }
    return rv;
}

int main(int argc, char **argv)
{
    int width, height;
    printf("[i] image_val: started\n");

    if(argc < 3) {
        printf("[-] image_val: using default width and height (2000x2000)\n");
        width = height = 2000;
    } else {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }

    queue_t* queue_from_conv = queue_acquire("tmp_QUEUE_VAL_CONV", QUEUE_SLAVE);
    queue_t* queue_to_hyp = queue_acquire("tmp_QUEUE_HYP_VAL", QUEUE_MASTER);
    if(queue_from_conv == NULL || queue_to_hyp == NULL) {
        printf("[-] image_val: Failed to get queues\n");
        return 1;
    }

    const size_t expected_packet_size = sizeof(struct packet) + IMAGE_SIZE(width, height);

    for(int i = 0; 1; i++)
    {
        struct packet* packet;
        size_t packet_size;

        struct timespec start;
        clock_gettime(CLOCK_REALTIME, &start);

        int ret = queue_sync_read(queue_from_conv, (void**) &packet, &packet_size);
        if(ret != 0) {
            printf("[-] image_val: failed to read from conv queue\n");
            continue;
        }

        if(packet_size != expected_packet_size) {
            printf("[-] image_val: invalid size of incomming packet - Got: %zu; Expected: %zu\n", 
                    packet_size, expected_packet_size);
            continue;
        } else
            printf("[i] image_val: Got packet of size %zu\n", packet_size);
        

        int rv = validate(packet, width, height);
        printf("[i] image_val: RV value %d\n", rv);

        // Update times
        set_start_time(packet, STAGE_R_VALIDATOR, &start);
        set_stop_time_now(packet, STAGE_R_VALIDATOR);

        // Send copy to hypervisor
        ret = queue_async_write(queue_to_hyp, packet, packet_size);
        if(ret != 0)
            printf("[-] image_val: failed to send packet to hypervisor\n");

        free(packet);
    }
    return 0;
}
