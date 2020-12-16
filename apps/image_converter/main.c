#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>
#include "image.h"
#include "queue.h"
#include "fastlz.h"
#include "device.h"

int main(int argc, char **argv)
{
    int width, height, shared_or_queue, trans_or_ret;
    size_t packet_size, message_size;
    int image_size, pixels_size, new_packet_size, new_new_packet_size;
    queue_t* queue_ptr;
    if(argc<3)
    {
        printf("Not enough arguments\n");
        return 1;
    }
    else if(argc<5)
    {
        printf("Not enough arguments. We will use default width and height\n");
        shared_or_queue = atoi(argv[1]);
        if(strcmp(argv[2],"mode=TRANSMITTER") == 0)
            trans_or_ret = 0;
        else
            trans_or_ret = 1;
        width = height = 2000;
    }
    else
    {
        shared_or_queue = atoi(argv[1]);
        if(strcmp(argv[2],"mode=TRANSMITTER") == 0)
            trans_or_ret = 0;
        else
            trans_or_ret = 1;
        width = atoi(argv[3]);
        height = atoi(argv[4]);
    }
    pixels_size = width*height*sizeof(struct pixel);
    image_size = 2*sizeof(int) + pixels_size;
    packet_size = 2*sizeof(struct timespec) + image_size;
    new_packet_size = 2*sizeof(struct timespec) + packet_size;
    new_new_packet_size = 2*sizeof(struct timespec) + new_packet_size;
    message_size = sizeof(struct packet*);
    // Now we "split" program into 2 sub-programms.
    // trans_or_ret == 0 means that we are in transmitter mode
    if(trans_or_ret == 0)
    {
        char* queue_name = "tmp_QUEUE_GEN_CONV";    // from init.c
        queue_ptr = queue_acquire(queue_name, QUEUE_SLAVE);
        struct ir_device* arduino;
        arduino = ir_open(NULL); // TODO - I DON'T KNOW THE PATH
        while(1)
        {
            struct timespec start = {0}, stop = {0};
            clock_gettime(CLOCK_REALTIME, &start);
            void* buffer;
            // shared_or_queue will be 0 if we want to transmit all data thru queue
            if(shared_or_queue == 0)
            {
                int ret = queue_sync_read(queue_ptr, &buffer, &packet_size);
                if(ret != 0) {
                    printf("err_ret = %d\n", ret);
                    continue;
                }
            }
            else
            {
                int ret = queue_sync_read(queue_ptr, &buffer, &message_size);
                if(ret != 0) {
                    printf("err_ret = %d\n", ret);
                    continue;
                }
            }
            void *compress_buffer, *decompress_buffer;
            compress_buffer = malloc(packet_size);
            decompress_buffer = malloc(packet_size);
            memcpy(decompress_buffer, buffer, packet_size);
            for(int i=0; i<5; i++)
            {
                int rv = fastlz_compress_level(2, (void*)decompress_buffer, packet_size, compress_buffer);
                rv = fastlz_decompress(compress_buffer, rv, decompress_buffer, packet_size);
            }
            struct packet* for_arduino;
            for_arduino = malloc(new_packet_size);
            memcpy((*for_arduino).data, buffer, packet_size);
            (*for_arduino).start = start;
            clock_gettime(CLOCK_REALTIME, &stop);
            (*for_arduino).stop = stop;
            if(new_packet_size != ir_write(arduino, (char*)for_arduino, new_packet_size))
                printf("We failed to sent our packet to Arduino\n");
            if(shared_or_queue == 0)
                free(buffer);
            else
                munmap(buffer, packet_size);
            printf("Message processed\n");
        }
        ir_close(arduino);
    }
    else
    {
        char* queue_name = "tmp_QUEUE_VAL_CONV";    // from init.c
        queue_ptr = queue_acquire(queue_name, QUEUE_MASTER);
        struct ir_device* arduino;
        arduino = ir_open(NULL); // TODO - I DON'T KNOW THE PATH
        while(1)
        {
            void *buffer;
            struct timespec start = {0}, stop = {0};
            clock_gettime(CLOCK_REALTIME, &start);
            buffer = malloc(new_packet_size);
            if(new_packet_size != ir_read(arduino, (char*)buffer, new_packet_size))
            {
                printf("We failed to read our packet from Arduino\n");
                continue;
            }
            void *compress_buffer, *decompress_buffer;
            compress_buffer = malloc(new_packet_size);
            decompress_buffer = malloc(new_packet_size);
            memcpy(decompress_buffer, buffer, new_packet_size);
            for(int i=0; i<5; i++)
            {
                int rv = fastlz_compress_level(2, (void*)decompress_buffer, new_packet_size, compress_buffer);
                rv = fastlz_decompress(compress_buffer, rv, decompress_buffer, new_packet_size);
            }
            struct packet* for_validator;
            for_validator = malloc(new_new_packet_size);
            memcpy((*for_validator).data, buffer, new_packet_size);
            free(buffer);
            (*for_validator).start = start;
            clock_gettime(CLOCK_REALTIME, &stop);
            (*for_validator).stop = stop;
            if(shared_or_queue == 0)
            {
                queue_sync_write(queue_ptr, (char*)for_validator, new_new_packet_size);
                free(for_validator);
            }
            else
            {
                void *data = (struct packet*) create_shared_memory(new_new_packet_size);
                memcpy(data, for_validator, new_new_packet_size);
                queue_sync_write(queue_ptr, (char*)&data, sizeof(char*));
            }
        }
    }
    return 0;
}
