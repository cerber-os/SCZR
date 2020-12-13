/*
 * Custom initialization process starting all neccessary
 * applications
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"

#define ARRAY_SIZE(X)      (sizeof(X) / sizeof(X[0]))

enum init_mode {
    INIT_MODE_TRANSMITTER = 0,
    INIT_MODE_RECEIVER = 1
};
struct subprocess {
    int id;
    const char* path;
    const char* args[3];
};
struct proc_queue {
    int from;
    int to;
    const char* name;
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
        {.from = PROC_HYPERVISOR, .to = PROC_IMAGE_GEN, .name="HYP_GEN"},
        {.from = PROC_HYPERVISOR, .to = PROC_IMAGE_CONV, .name="HYP_CONV"},
        {.from = PROC_IMAGE_GEN,  .to = PROC_IMAGE_CONV, .name="GEN_CONV"},
    },
    [INIT_MODE_RECEIVER] = {
        {.from = PROC_HYPERVISOR, .to = PROC_IMAGE_VAL, .name="HYP_VAL"},
        {.from = PROC_HYPERVISOR, .to = PROC_IMAGE_CONV, .name="HYP_CONV"},
        {.from = PROC_IMAGE_VAL,  .to = PROC_IMAGE_CONV, .name="VAL_CONV"},
    }
};

#define INIT_PROCESSES_COUNT    3
static const struct subprocess subprocesses[][INIT_PROCESSES_COUNT] = {
    [INIT_MODE_TRANSMITTER] = {
        {.id = PROC_HYPERVISOR, .path="hypervisor", .args={"mode=TRANSMITTER", 0}},
        {.id = PROC_IMAGE_GEN, .path="image_gen", .args={0}},
        {.id = PROC_IMAGE_CONV, .path="image_conv", .args={"mode=TRANSMITTER", 0}},
    },
    [INIT_MODE_RECEIVER] = {
        {.id = PROC_HYPERVISOR, .path="hypervisor", .args={"mode=RECEIVER", 0}},
        {.id = PROC_IMAGE_VAL, .path="image_val", .args={0}},
        {.id = PROC_IMAGE_CONV, .path="image_conv", .args={"mode=RECEIVER", 0}},
    }
};


static int error_exit(int error_code) {
    exit(error_code);
}


static void low_level_init(void) {
    static const char* init_commands[] = {
        // Mount basic filesystems
        "mount -t tmpfs none /tmp",
        "mount -t proc none /proc",
        "mount -t sysfs none /sys",

        // pts is requires for Xorg
        "mkdir /dev/pts",
        "mount -t devpts none /dev/pts",
    };

    for(int i = 0; i < ARRAY_SIZE(init_commands); i++) {
        int ret = system(init_commands[i]);
        if(ret != 0) {
            fprintf(stderr, "Warning: Command '%s' failed with error %d\n", init_commands[i], ret);
        }
    }
}


int main(int argc, char** argv) {
    enum init_mode mode = INIT_MODE_TRANSMITTER;

    // Parse program options
    for(int i = 1; i < argc; i++) {

        if(!strncmp(argv[i], "mode=", sizeof("mode=") - 1)) {
            const char* opt = argv[i] + sizeof("mode=") - 1;
            if(!strcasecmp(opt, "TRANSMITTER"))
                mode = INIT_MODE_TRANSMITTER;
            else if(!strcasecmp(opt, "RECEIVER"))
                mode = INIT_MODE_RECEIVER;
            else {
                fprintf(stderr, "Invalid mode provided - '%s'. Expected either TRANSMITTER or RECEIVER\n", opt);
                error_exit(1);
            }
        } else {
            fprintf(stderr, "Invalid option provided - '%s'\n", argv[i]);
            error_exit(1);
        }
    }

    // If we're running as root (QEMU scenario), perform low level init
    if(getuid() == 0) {
        low_level_init();
    }

    // Setup queues
    for(int i = 0; i < INIT_QUEUES_COUNT; i++) {
        const struct proc_queue* q = &queues[mode][i];
        char path[256];
        strcpy(path, "/tmp_QUEUE_");
        strcpy(path + sizeof("/tmp_QUEUE_") - 1, q->name);

        int ret = queue_create(path, INIT_QUEUE_SIZE);
        if(ret < 0) {
            fprintf(stderr, "Failed to setup queue - '%s'\n", path);
            error_exit(1);
        }
    }

    // Start processes
    for(int i = 0; i < INIT_PROCESSES_COUNT; i++) {
        int pid = fork();
        if(pid == 0) {
            execve(subprocesses[mode][i].path, (char *const*)subprocesses[mode][i].args, NULL);
        } else if (pid < 0) {
            perror("[-] init: fork failed");
            error_exit(1);
        }
    }

    // Wait endlessly
    while(1)
        sleep(10000);
}
