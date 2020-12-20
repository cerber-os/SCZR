#include <assert.h>
#include <string.h>
#include <time.h>

#include "misc.h"

const int PACKET_MAGIC_VALUE = 0xffaaccbb;
const char* PACKET_MAGIC_VALUE_CHAR = (char*) &PACKET_MAGIC_VALUE;

void set_start_time_now(struct packet* packet, unsigned int stage_no) {
    assert(stage_no < NUMBER_OF_STAGES);
    clock_gettime(CLOCK_REALTIME, &packet->timestamps[stage_no].start);
}

void set_stop_time_now(struct packet* packet, unsigned int stage_no) {
    assert(stage_no < NUMBER_OF_STAGES);
    clock_gettime(CLOCK_REALTIME, &packet->timestamps[stage_no].stop);
}


void set_start_time(struct packet* packet, unsigned int stage_no, struct timespec* time) {
    assert(stage_no < NUMBER_OF_STAGES);
    memcpy(&packet->timestamps[stage_no].start, time, sizeof(struct timespec));
}

void set_stop_time(struct packet* packet, unsigned int stage_no, struct timespec* time) {
    assert(stage_no < NUMBER_OF_STAGES);
    memcpy(&packet->timestamps[stage_no].stop, time, sizeof(struct timespec));
}


static long time_diff_us(struct timespec* start, struct timespec* end) {
    return (end->tv_sec * 1000 * 1000 + end->tv_nsec / 1000) - \
        (start->tv_sec * 1000 * 1000 + start->tv_nsec / 1000);
}

/*
static long time_diff_ms(struct timespec* start, struct timespec* end) {
    return (end->tv_sec * 1000 + end->tv_nsec / 1000 / 1000) - \
        (start->tv_sec * 1000 + start->tv_nsec / 1000 / 1000);
}
*/

long time_spent_in_one_stage_us(struct packet* packet, unsigned int stage_no) {
    assert(stage_no < NUMBER_OF_STAGES);

    return time_diff_us(&packet->timestamps[stage_no].start, &packet->timestamps[stage_no].stop);
}

long time_spent_in_stages_us(struct packet* packet, unsigned int stage_start, unsigned int stage_end) {
    assert(stage_start < NUMBER_OF_STAGES);
    assert(stage_end < NUMBER_OF_STAGES);

    return time_diff_us(&packet->timestamps[stage_start].stop, &packet->timestamps[stage_end].start);
}

long time_between_stages_us(struct packet* packet, unsigned int stage_start, unsigned int stage_end) {
    assert(stage_start < NUMBER_OF_STAGES);
    assert(stage_end < NUMBER_OF_STAGES);

    return time_diff_us(&packet->timestamps[stage_start].start, &packet->timestamps[stage_end].stop);
}

long time_start_stage_us(struct packet* packet, unsigned int stage_no) {
    assert(stage_no < NUMBER_OF_STAGES);
    struct timespec* start = &packet->timestamps[stage_no].start;

    return start->tv_sec * 1000 * 1000 + start->tv_nsec / 1000;
}

long time_stop_stage_us(struct packet* packet, unsigned int stage_no) {
    assert(stage_no < NUMBER_OF_STAGES);
    struct timespec* end = &packet->timestamps[stage_no].stop;

    return end->tv_sec * 1000 * 1000 + end->tv_nsec / 1000;
}
