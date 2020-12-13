/*
 * Implementation of process visualizing other apps performance
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"

int main(int argc, char** argv) {
    queue_t* q_hyp_gen = queue_acquire("tmp_QUEUE_HYP_GEN", QUEUE_MASTER);
    if(q_hyp_gen == NULL) {
        fprintf(stderr, "Failed to acquire queue\n");
        exit(1);
    }
    printf("Got buffer\n");

    int ret = queue_sync_write(q_hyp_gen, "abcdef", 6);
    printf("Got: %d\n", ret);

    ret = queue_async_write(q_hyp_gen, "abcdef", 6);
    printf("Got2: %d\n", ret);

    char* buffer; size_t size;
    ret = queue_async_read(q_hyp_gen, &buffer, &size);
    printf("Got2: %d %p %ld\n", ret, buffer, size);

    printf("Will hang...\n");
    fflush(stdout);

    ret = queue_sync_read(q_hyp_gen, &buffer, &size);
    printf("Got2: %d %p %ld\n", ret, buffer, size);
}
