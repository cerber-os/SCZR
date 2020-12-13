/*
 * Implementation of synchronized, two-way communication pipe
 * based on two queues
 */

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define QUEUE_MMAP_ADDR     (void*)0xcaffe000
#define QUEUE_WRITE_MARGIN  50

enum queue_error {
    QUEUE_E2BIG     = -10,
    QUEUE_EMEM,
    QUEUE_EINVAL,
    QUEUE_EFAULT,
    QUEUE_ERETRY,
    QUEUE_OK        = 0,
};

enum queue_mode {
    QUEUE_MASTER,
    QUEUE_SLAVE,
};

struct queue_element {
    // struct queue_element* prev;
    struct queue_element* next;
    size_t size;
    char data[];
};

struct queue_header {
    size_t full_size;

    struct queue_body {
        pthread_mutex_t lock;
        sem_t sem_packets;
        struct queue_element* first;
        struct queue_element* last;
        char* data;
        size_t data_size;
    } queue_a, queue_b;
};

typedef struct queue {
    int mode;
    char path[255];
    struct queue_header* header;
} queue_t;



/* 
 * Create a new queue represented by file `path` 
 */
int queue_create(const char* path, size_t size);

/*
 * Delete queue associated with provided file
 */
int queue_delete(const char* path);

/*
 * Get queue object associated with provided file
 */
queue_t* queue_acquire(const char* path, int mode);

void queue_free(queue_t* queue);

int queue_sync_read(queue_t* queue, char** buffer, size_t* outSize);

int queue_async_read(queue_t* queue, char** buffer, size_t* outSize);

int queue_sync_write(queue_t* queue, char* buffer, size_t size);

int queue_async_write(queue_t* queue, char* buffer, size_t size);
