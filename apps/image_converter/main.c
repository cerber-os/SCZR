#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>

#include "image.h"
#include "queue.h"
#include "fastlz.h"
#include "device.h"
#include "misc.h"

int main(int argc, char **argv)
{
    size_t width, height, trans_or_ret;

    printf("[i] image_conv: started\n");

    if(argc < 2) {
        printf("[i] image_conv: missing mode parameter\n");
        return 1;
    }

    if(strcmp(argv[1],"mode=TRANSMITTER") == 0)
        trans_or_ret = 0;
    else
        trans_or_ret = 1;

    if(argc < 4) {
        printf("[i] image_conv: using default width and height (2000x2000)\n");
        width = height = 2000;
    } else {
        width = atoi(argv[2]);
        height = atoi(argv[3]);
    }

    // Try to open our remote device
    ir_device* arduino = ir_open("/dev/ttyACM0");
    if(arduino == NULL) {
        printf("[i] image_conv: Failed to open arduino ACM0\n");

        arduino = ir_open("/dev/ttyUSB0");
        if(arduino == NULL) {
            printf("[i] image_conv: Failed to open arduino USB0\n");
            printf("[-] image_conv: both attempts to open node failed\n");
            // Try to continue program execution...
        }
    }

    struct packet* packet;
    size_t packet_size;
    const size_t expected_packet_size = IMAGE_SIZE(width, height) + sizeof(struct packet);
    const size_t header_size = sizeof(struct packet) - 4*sizeof(char);
    const size_t image_size = IMAGE_SIZE(width, height);

    // Now we "split" program into 2 sub-programms.
    // trans_or_ret == 0 means that we are in transmitter mode
    if(trans_or_ret == 0) {
        queue_t* queue_from_gen = queue_acquire("tmp_QUEUE_GEN_CONV", QUEUE_SLAVE);
        queue_t* queue_to_hyp = queue_acquire("tmp_QUEUE_HYP_CONV", QUEUE_MASTER);
        if(queue_from_gen == NULL || queue_to_hyp == NULL) {
            printf("[-] image_conv: Failed to open queues\n");
            return 1;
        }
        char* compress_buffer = malloc(image_size);
        if(compress_buffer == NULL) {
            printf("[-] image_conv: failed to allocate compress buffers\n");
            return 1;
        }

        for(int i = 0; 1; i++)
        {
            struct timespec start;
            clock_gettime(CLOCK_REALTIME, &start);

            int ret = queue_sync_read(queue_from_gen, (void**) &packet, &packet_size);
            if(ret != 0) {
                printf("[-] image_conv: queue_sync_read: err_ret = %d\n", ret);
                continue;
            }
            if(packet_size != expected_packet_size) {
                printf("[-] image_conv: invalid packet size - Got: %zu; expected: %zu\n", packet_size, expected_packet_size);
                free(packet);
                continue;
            }
            
            {
                char *image_ptr = packet->data;
                int compressed_buffer_size = packet_size;
                for(int i = 0; i < 5; i++)
                {
                    void *ptr;
                    compressed_buffer_size = fastlz_compress_level(2, (void*)image_ptr, compressed_buffer_size, (void*)compress_buffer);
                    ptr = image_ptr;
                    image_ptr = compress_buffer;
                    compress_buffer = ptr;
                }

                if (packet->data != image_ptr)
                    memcpy(packet->data, image_ptr, compressed_buffer_size);

                packet_size = compressed_buffer_size + sizeof(struct packet);
                packet->compressed_buffer_size = compressed_buffer_size;
            }

            set_start_time(packet, STAGE_T_CONV, &start);
            set_stop_time_now(packet, STAGE_T_CONV);
            
            size_t already_sent = 0;
            while(already_sent < packet_size)
                already_sent += ir_write(arduino, 
                        (char*)packet + already_sent, 
                        (packet_size - already_sent > 100) ? 100: packet_size - already_sent);

            printf("[i] image_conv: Message processed - bytes (%zu)\n", packet_size);

            ret = queue_async_write(queue_to_hyp, packet, packet_size);
            if(ret != 0)
                printf("[-] image_conv: failed to send packet to hypervisor\n");
            
            free(packet);
        }
        free(compress_buffer);
    } else {
        queue_t* queue_to_val = queue_acquire("tmp_QUEUE_VAL_CONV", QUEUE_MASTER);
        if(queue_to_val == NULL) {
            printf("[-] image_conv: Failed to open queue `tmp_QUEUE_VAL_CONV`\n");
            return 1;
        }
        char* decompress_buffer = malloc(image_size);
        // Add 10 bytes to size - just for safety
        char* arduino_input = malloc(expected_packet_size + 10);
        if(decompress_buffer == NULL || arduino_input == NULL) {
            printf("[-] image_conv: failed to allocate buffers\n");
            return 1;
        }

        struct packet* packet = (struct packet*) arduino_input;
        
        for(int i = 0; 1; i++)
        {
            struct timespec start, start_processing, stop;
            int compressed_buffer_size, found_start = 0;
            
            while(found_start < 4) {
                char test;
                ir_read(arduino, &test, 1);
                if(test == PACKET_MAGIC_VALUE_CHAR[found_start])
                    found_start++;
                else if(test == PACKET_MAGIC_VALUE_CHAR[0])
                    found_start = 1;
                else
                    found_start = 0;
            }
            size_t part_size = 4;

            if(header_size == ir_read(arduino, (char*)arduino_input + part_size, header_size))
            {
                compressed_buffer_size = packet->compressed_buffer_size;
                part_size += header_size;
            }
            else
                continue;          

            // Start time messurement after receiving the whole correct header
            clock_gettime(CLOCK_REALTIME, &start);

            while(part_size < compressed_buffer_size)
                part_size += ir_read(arduino, (char*)arduino_input + part_size, compressed_buffer_size - part_size);
            
            clock_gettime(CLOCK_REALTIME, &start_processing);
        
            {
                char *image_ptr = packet->data;
                for(int i = 0; i < 5; i++)
                {
                    void *ptr;
                    compressed_buffer_size = fastlz_decompress((void*)image_ptr, compressed_buffer_size, (void*)decompress_buffer, image_size);
                    ptr = image_ptr;
                    image_ptr = decompress_buffer;
                    decompress_buffer = ptr;
                }

                if(compressed_buffer_size != image_size)
                {
                    printf("[-] image_conv: failed to decompress image\n");
                    continue;
                }

                if (packet->data != image_ptr)
                    memcpy(packet->data, image_ptr, compressed_buffer_size);
            }
            
            set_start_time(packet, STAGE_R_CONV, &start);
            set_start_time(packet, STAGE_R_CONV_PROC, &start_processing);
            clock_gettime(CLOCK_REALTIME, &stop);
            set_stop_time(packet, STAGE_R_CONV, &stop);
            set_stop_time(packet, STAGE_R_CONV_PROC, &stop);

            queue_sync_write(queue_to_val, (char*)packet, expected_packet_size);
        }

        free(arduino_input);
        free(decompress_buffer);
    }
    return 0;
}
