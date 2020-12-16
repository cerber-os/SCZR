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

    // Now we "split" program into 2 sub-programms.
    // trans_or_ret == 0 means that we are in transmitter mode
    if(trans_or_ret == 0) {
        queue_t* queue_from_gen = queue_acquire("tmp_QUEUE_GEN_CONV", QUEUE_SLAVE);
        queue_t* queue_to_hyp = queue_acquire("tmp_QUEUE_HYP_CONV", QUEUE_MASTER);
        if(queue_from_gen == NULL || queue_to_hyp == NULL) {
            printf("[-] image_conv: Failed to open queues\n");
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
                char* compress_buffer = malloc(packet_size);
                char* decompress_buffer = malloc(packet_size);
                if(compress_buffer == NULL || decompress_buffer == NULL) {
                    printf("[-] image_conv: failed to allocate compress buffers\n");
                    continue;
                }
                memcpy(decompress_buffer, packet, packet_size);

                for(int i = 0; i < 5; i++)
                {
                    int rv = fastlz_compress_level(2, (void*)decompress_buffer, packet_size, (void*)compress_buffer);
                    rv = fastlz_decompress((void*)compress_buffer, rv, (void*)decompress_buffer, packet_size);
                }

                free(decompress_buffer);
                free(compress_buffer);
            }

            set_start_time(packet, STAGE_T_CONV, &start);
            set_stop_time_now(packet, STAGE_T_CONV);
            
            if(packet_size != ir_write(arduino, packet, packet_size))
                printf("[-] image_conv: failed to sent our packet to Arduino\n");

            printf("[i] image_conv: Message processed - bytes (%zu)\n", packet_size);

            ret = queue_async_write(queue_to_hyp, packet, packet_size);
            if(ret != 0)
                printf("[-] image_conv: failed to send packet to hypervisor\n");
            
            free(packet);
        }
    } else {
        queue_t* queue_to_val = queue_acquire("tmp_QUEUE_VAL_CONV", QUEUE_MASTER);
        if(queue_to_val == NULL) {
            printf("[-] image_conv: Failed to open queue `tmp_QUEUE_VAL_CONV`\n");
            return 1;
        }

        // Add 10 bytes to size - just for safety
        char* arduino_input = malloc(expected_packet_size + 10);

        for(int i = 0; 1; i++)
        {
            struct timespec start;
            clock_gettime(CLOCK_REALTIME, &start);

            size_t part_size = 0;
            while(part_size < expected_packet_size)
                part_size += ir_read(arduino, (char*)arduino_input + part_size, expected_packet_size - part_size);
        
            {
                char* compress_buffer = malloc(expected_packet_size);
                char* decompress_buffer = malloc(expected_packet_size);
                if(compress_buffer == NULL || decompress_buffer == NULL) {
                    printf("[-] image_conv: failed to allocate compress buffers\n");
                    continue;
                }
                memcpy(decompress_buffer, arduino_input, expected_packet_size);
                for(int i=0; i<5; i++)
                {
                    int rv = fastlz_compress_level(2, (void*)decompress_buffer, expected_packet_size, (void*)compress_buffer);
                    rv = fastlz_decompress((void*)compress_buffer, rv, (void*)decompress_buffer, expected_packet_size);
                }
                free(decompress_buffer);
                free(compress_buffer);
            }

            struct packet* packet = (struct packet*) arduino_input;
            set_start_time(packet, STAGE_R_CONV, &start);
            set_stop_time_now(packet, STAGE_R_CONV);

            queue_sync_write(queue_to_val, (char*)packet, expected_packet_size);
        }
        free(arduino_input);
    }
    return 0;
}
