#include <time.h>

enum {
    STAGE_T_GENERATOR = 0,
    STAGE_T_CONV,
    STAGE_R_CONV,
    STAGE_R_CONV_PROC,
    STAGE_R_VALIDATOR,

    NUMBER_OF_STAGES
};

extern const int PACKET_MAGIC_VALUE;
extern const char* PACKET_MAGIC_VALUE_CHAR;

struct packet {
    char magic[4];
    struct period {
        struct timespec start;
        struct timespec stop;
    } timestamps[NUMBER_OF_STAGES];
    int valid_result;
    int compressed_buffer_size; 

    char data[0];
} __attribute__((packed));

void set_start_time_now(struct packet*, unsigned int);
void set_stop_time_now(struct packet*, unsigned int);
void set_start_time(struct packet*, unsigned int, struct timespec*);
void set_stop_time(struct packet*, unsigned int, struct timespec*);
long time_spent_in_one_stage_us(struct packet*, unsigned int);
long time_spent_in_stages_us(struct packet*, unsigned int, unsigned int);
long time_between_stages_us(struct packet*, unsigned int, unsigned int);
long time_start_stage_us(struct packet*, unsigned int);
long time_stop_stage_us(struct packet*, unsigned int);
