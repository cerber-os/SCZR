/*
 * Custom initialization process starting all neccessary
 * applications
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"

#define ARRAY_SIZE(X)      (sizeof(X) / sizeof(X[0]))

/*
 * Program config
 */
#define IMAGE_WIDTH_S        "160"
#define IMAGE_HEIGHT_S       "80"
#define IMAGE_DIM_S          "160","80"


enum init_mode {
    INIT_MODE_TRANSMITTER = 0,
    INIT_MODE_RECEIVER = 1
};
struct subprocess {
    int id;
    const char* path;
    const char* args[7];
};
struct proc_queue {
    int from;
    int to;
    const char* name;
    int uid;
};

/*
 * Processes list and their queues
 */
enum process_ids {
    PROC_HYPERVISOR = 0,
    PROC_IMAGE_GEN,
    PROC_IMAGE_VAL,
    PROC_IMAGE_CONV,
};

#define INIT_QUEUE_SIZE     (10 * 1024 * 1024)      // 10 MB
#define INIT_QUEUES_COUNT    3
static const struct proc_queue queues[][INIT_QUEUES_COUNT] = {
    [INIT_MODE_TRANSMITTER] = {
        {.uid = 0, .from = PROC_HYPERVISOR, .to = PROC_IMAGE_GEN, .name="HYP_GEN"},
        {.uid = 1, .from = PROC_HYPERVISOR, .to = PROC_IMAGE_CONV, .name="HYP_CONV"},
        {.uid = 2, .from = PROC_IMAGE_GEN,  .to = PROC_IMAGE_CONV, .name="GEN_CONV"},
    },
    [INIT_MODE_RECEIVER] = {
        {.uid = 0, .from = PROC_HYPERVISOR, .to = PROC_IMAGE_VAL, .name="HYP_VAL"},
        {.uid = 1, .from = PROC_HYPERVISOR, .to = PROC_IMAGE_CONV, .name="HYP_CONV"},
        {.uid = 2, .from = PROC_IMAGE_VAL,  .to = PROC_IMAGE_CONV, .name="VAL_CONV"},
    }
};

#define INIT_PROCESSES_COUNT    3
static const struct subprocess subprocesses[][INIT_PROCESSES_COUNT] = {
    [INIT_MODE_TRANSMITTER] = {
        {.id = PROC_HYPERVISOR, .path="hypervisor", .args={"hypervisor", "mode=TRANSMITTER", IMAGE_DIM_S}},
        {.id = PROC_IMAGE_GEN, .path="image_generator", .args={"image_generator", IMAGE_DIM_S}},
        {.id = PROC_IMAGE_CONV, .path="image_converter", .args={"image_converter", "mode=TRANSMITTER", IMAGE_DIM_S}},
    },
    [INIT_MODE_RECEIVER] = {
        {.id = PROC_HYPERVISOR, .path="hypervisor", .args={"hypervisor", "mode=RECEIVER", IMAGE_DIM_S}},
        {.id = PROC_IMAGE_VAL, .path="image_validator", .args={"image_val", IMAGE_DIM_S}},
        {.id = PROC_IMAGE_CONV, .path="image_converter", .args={"image_conv", "mode=RECEIVER", IMAGE_DIM_S}},
    }
};

extern char** environ;


int main(int argc, char** argv) {
    char cmdline[1024];
    enum init_mode mode = INIT_MODE_TRANSMITTER;
    enum queue_mem_mode queue_mode = QUEUE_LOCAL;

    printf("[i] init: started\n");

    // Check mode
    int fd = open("/proc/cmdline", O_RDONLY, 0);
    if(fd < 0) {
        perror("[-] init: open /proc/cmdline failed");
        exit(1);
    }
    read(fd, cmdline, sizeof(cmdline));
    close(fd);

    // Check mode
    printf("[i] init: cmdline - `%s`\n", cmdline);
    if(strstr(cmdline, "mode=TRANSMITTER"))
        mode = INIT_MODE_TRANSMITTER;
    else if(strstr(cmdline, "mode=RECEIVER"))
        mode = INIT_MODE_RECEIVER;
    else
        printf("[-] init: unknown mode - continue with transmitter\n");
    printf("[i] init: continue with mode %d\n", mode);

    // Check queue config
    if(strstr(cmdline, "queue=local"))
        queue_mode = QUEUE_LOCAL;
    else if(strstr(cmdline, "queue=shared"))
        queue_mode = QUEUE_SHARED;
    else
        printf("[-] init: unknown queue mode - continue with local\n");
    printf("[i] init: using queue mode = %d\n", queue_mode);

    // Setup queues
    printf("[i] init: setting up queues\n");
    for(int i = 0; i < INIT_QUEUES_COUNT; i++) {
        const struct proc_queue* q = &queues[mode][i];
        char path[256];
        strcpy(path, "/tmp_QUEUE_");
        strcpy(path + sizeof("/tmp_QUEUE_") - 1, q->name);

        int ret = queue_create(path, INIT_QUEUE_SIZE, q->uid, queue_mode);
        if(ret < 0) {
            fprintf(stderr, "Failed to setup queue - '%s'\n", path);
            exit(1);
        } else {
            printf("[i] init: Created queue %s\n", path);
        }
    }

    if(argc >= 2 && strstr(argv[1], "skip_spawn")) {
        printf("[i] init: Queues created - skipping processes spawn\n");
        return 0;
    }

    // Start processes
    for(int i = 0; i < INIT_PROCESSES_COUNT; i++) {
        int pid = fork();
        if(pid == 0) {
            printf("[i] init: starting %s...\n", subprocesses[mode][i].path);
            execve(subprocesses[mode][i].path, (char *const*)subprocesses[mode][i].args, environ);

            printf("[i] init: failed to start %s \n", subprocesses[mode][i].path);
            exit(1);
        } else if (pid < 0) {
            perror("[-] init: fork failed\n");
            exit(1);
        }
    }

    // Wait endlessly
    while(1)
        sleep(10000);
}
