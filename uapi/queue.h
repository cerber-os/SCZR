/*
 * Implementation of synchronized, two-way communication pipe
 * based on two queues
 */

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define QUEUE_WRITE_MARGIN  50

enum queue_error {
    QUEUE_E2BIG     = -10,      // Provided message is too big to fit currently into queue
    QUEUE_EMEM,                 // OOM
    QUEUE_EINVAL,               // Invalid argument has been provided
    QUEUE_EFAULT,               // General fault
    QUEUE_ERETRY,               // Try again (used in *_async_* methods)
    QUEUE_OK        = 0,
};

// Specified the mode of queue - one side MUST be master and
//  the other one slave
enum queue_mode {
    QUEUE_MASTER,
    QUEUE_SLAVE,
};

/********************************************
 * QUEUE structures
 ********************************************/
// Represents single element pushed into queue
struct queue_element {
    struct queue_element* next;
    size_t size;
    char data[];
};

// Represents allocated two-way queue
struct queue_header {
    int uid;
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

// Local representation used by UAPI to reference to particular queue
typedef struct queue {
    int mode;
    char path[255];
    struct queue_header* header;
} queue_t;


/********************************************
 * QUEUE unified API
 ********************************************/

/* 
 * Create a new queue
 *  @path: unique name of the queue - must met requirement for shm_open
 *  @size: size in bytes of queues buffer
 *  @uid:  unique identifier of queue - must be unique for all processes on VM
 *  @return: QUEUE_OK on success, negative otherwise
 */
int queue_create(const char* path, size_t size, int uid);

/*
 * Delete queue
 *  @path: unique name of the created queue
 *  @return: QUEUE_OK on success, negative otherwise
 */
int queue_delete(const char* path);

/*
 * Get queue object
 *  @path: unique name of the queue
 *  @mode: one of the values specified in enum queue_mode.
 *         MUST be different for both sides of queue
 *  @return: pointer to allocated structure representing queue
 */
queue_t* queue_acquire(const char* path, int mode);

/* 
 * Release queue object
 *  @queue: pointer to queue object to be released
 */
void queue_free(queue_t* queue);

/* 
 * Synchronously read from queue, i.e.: if no packet is present on queue,
 * wait for it to arrive.
 *  @queue: pointer to queue object
 *  @buffer: address of variable to which pointer to new buffer will be saved.
 *           It is callee duty to FREE memory after use !!!
 *  @outSize: address of variable to which size of `buffer` will be saved
 *  @return: QUEUE_OK on success, negative otherwise
 */
int queue_sync_read(queue_t* queue, void** buffer, size_t* outSize);

/* 
 * Asynchronously read from queue - don't wait for packet
 *  @queue: pointer to queue object
 *  @buffer: address of variable to which pointer to new buffer will be saved.
 *           It is callee duty to FREE memory after use !!!
 *  @outSize: address of variable to which size of `buffer` will be saved
 *  @return: QUEUE_OK on success, QUEUE_ERETRY if no packet is available
 */
int queue_async_read(queue_t* queue, void** buffer, size_t* outSize);

/* 
 * Synchronously write to queue, i.e.: if queue is full, wait for free space
 *  @queue: pointer to queue object
 *  @buffer: buffer with data to write
 *  @size: size of buffer in bytes
 *  @return: QUEUE_OK on success, QUEUE_FAULT when packet is too large even for empty queue
 */
int queue_sync_write(queue_t* queue, void* buffer, size_t size);

/* 
 * Asynchronously write to queue - don't wait for free space
 *  @queue: pointer to queue object
 *  @buffer: buffer with data to write
 *  @size: size of buffer in bytes
 *  @return: QUEUE_OK on success, QUEUE_FAULT when packet is too large even for empty queue,
 *           QUEUE_ERETRY if queue is currently full
 */
int queue_async_write(queue_t* queue, void* buffer, size_t size);

/* 
 * Return number of messages present in SEND queue
 *  @queue: pointer to queue object
 *  @return: number of elements on SEND queue
 */
int queue_get_pending(queue_t* queue);
