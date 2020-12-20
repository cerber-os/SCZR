#include <time.h>

enum {
    STAGE_T_GENERATOR = 0,
    STAGE_T_CONV,
    STAGE_R_CONV,
    STAGE_R_VALIDATOR,

    NUMBER_OF_STAGES
};

struct packet {
    struct period {
        struct timespec start;
        struct timespec stop;
    } timestamps[NUMBER_OF_STAGES];

    char data[0];
};

void set_start_time_now(struct packet*, unsigned int);
void set_stop_time_now(struct packet*, unsigned int);
void set_start_time(struct packet* packet, unsigned int stage_no, struct timespec* time);
void set_stop_time(struct packet* packet, unsigned int stage_no, struct timespec* time);
long time_spent_in_one_stage_us(struct packet*, unsigned int);
long time_spent_in_stages_us(struct packet*, unsigned int, unsigned int);
long time_between_stages_us(struct packet*, unsigned int, unsigned int);
long time_start_stage_us(struct packet* packet, unsigned int stage_no);
long time_stop_stage_us(struct packet* packet, unsigned int stage_no);
