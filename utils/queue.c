/*
 * Implementation of synchronized, two-way communication pipe
 * based on two queues
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "queue.h"

int queue_create(const char* path, size_t size) {
    int memfd = shm_open(path, O_CREAT | O_RDWR, 0755);
    if(memfd < 0) {
        perror("[-] queue: shm_open failed");
        return QUEUE_EFAULT;
    }
    
    size_t memory_size = sizeof(struct queue_header) + size * 2;

    int ret = ftruncate(memfd, memory_size);
    if(ret < 0) {
        perror("[-] queue: ftruncate failed");
        close(memfd);
        return QUEUE_EFAULT;
    }

    void* addr = mmap(QUEUE_MMAP_ADDR, 
                    memory_size, 
                    PROT_READ | PROT_WRITE, 
                    MAP_SHARED, 
                    memfd, 0);
    if(addr == MAP_FAILED) {
        perror("[-] queue: mmap failed");
        close(memfd);
        return QUEUE_EFAULT;
    }

    // Initialize queue header
    struct queue_header* header = (struct queue_header*) addr;
    header->full_size = memory_size;

    pthread_mutex_init(&header->queue_a.lock, NULL);
    sem_init(&header->queue_a.sem_packets, 1, 0);
    header->queue_a.first = header->queue_a.last = NULL;
    header->queue_a.data_size = size;
    header->queue_b.data = addr + sizeof(*header);
    
    pthread_mutex_init(&header->queue_b.lock, NULL);
    sem_init(&header->queue_b.sem_packets, 1, 0);
    header->queue_b.first = header->queue_b.last = NULL;
    header->queue_b.data_size = size;
    header->queue_b.data = addr + sizeof(*header) + header->queue_a.data_size;

    munmap(addr, memory_size);
    close(memfd);
    return QUEUE_OK;
}


int queue_delete(const char* path) {
    int ret = shm_unlink(path);
    if(ret < 0)
        perror("[-] queue: shm_unlink failed");
    return ret;
}


queue_t* queue_acquire(const char* path, int mode) {
    queue_t* new_queue = malloc(sizeof(queue_t));
    if(new_queue == NULL) {
        perror("[-] queue: malloc failed");
        return (queue_t*) QUEUE_EMEM;
    }

    int memfd = shm_open(path, O_CREAT | O_RDWR, 0755);
    if(memfd < 0) {
        perror("[-] queue (acquire): shm_open failed");
        return (queue_t*) QUEUE_EFAULT;
    }

    // Mmap only header on shared memory
    size_t header_size = sizeof(struct queue_header);
    void* addr = mmap(QUEUE_MMAP_ADDR, header_size, 
                    PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if(addr == MAP_FAILED) {
        perror("[-] queue (acquire): mmap failed");
        close(memfd);
        return (queue_t*) QUEUE_EFAULT;
    }

    // Read size of queue memory and reallocate buffer
    size_t memory_size = ((struct queue_header*) addr)->full_size;
    munmap(addr, header_size);
    addr = mmap(QUEUE_MMAP_ADDR, memory_size, 
                    PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if(addr == MAP_FAILED) {
        perror("[-] queue (acquire): (re)mmap failed");
        close(memfd);
        return (queue_t*) QUEUE_EFAULT;
    }

    // Fill queue information structure
    strncpy(new_queue->path, path, sizeof(new_queue->path) - 1);
    new_queue->mode = mode;
    new_queue->header = addr;

    close(memfd);
    return new_queue;
}


void queue_free(queue_t* queue) {
    munmap(queue->header, queue->header->full_size);
    free(queue);
}

static struct queue_body* get_read_queue(struct queue_header* header, int mode) {
    if(mode == QUEUE_MASTER)
        return &header->queue_a;
    else
        return &header->queue_b;
}

static struct queue_body* get_write_queue(struct queue_header* header, int mode) {
    if(mode == QUEUE_MASTER)
        return &header->queue_b;
    else
        return &header->queue_a;
}

static int _queue_read(queue_t* queue, char** buffer, size_t* outSize) {
    struct queue_body* read_q = get_read_queue(queue->header, queue->mode);
    assert(read_q->first != NULL);

    char* el_addr = read_q->first->data;
    size_t el_size = read_q->first->size;
    *outSize = el_size;
    *buffer = malloc(*outSize);
    if(*buffer == NULL) {
        perror("[-] queue (sync_read): OOM");
        return QUEUE_EMEM;
    }

    if(el_addr + el_size < read_q->data + read_q->data_size) {
        memcpy(*buffer, el_addr, el_size);
    } else {
        size_t part_a_size = (size_t)(read_q->data + read_q->data_size) - (size_t)el_addr;
        if(part_a_size > el_size)
            part_a_size = el_size;
        size_t part_b_size = el_size - part_a_size;
        assert(part_a_size + part_b_size == el_size);

        memcpy(*buffer, el_addr, part_a_size);
        if(part_b_size)
            memcpy(*buffer + part_a_size, read_q->data, part_b_size);
    }

    if(read_q->last == read_q->first)
        read_q->last = NULL;
    read_q->first = read_q->first->next;
    return QUEUE_OK;
}


int queue_sync_read(queue_t* queue, char** buffer, size_t* outSize) {
    int ret = QUEUE_OK;
    struct queue_body* read_q = get_read_queue(queue->header, queue->mode);
    
    sem_wait(&read_q->sem_packets);

    pthread_mutex_lock(&read_q->lock);
    ret = _queue_read(queue, buffer, outSize);
    if(ret < 0)     // If error occurred, treat packet as unread
        sem_post(&read_q->sem_packets);
    pthread_mutex_unlock(&read_q->lock);
    return ret;
}


int queue_async_read(queue_t* queue, char** buffer, size_t* outSize) {
    int ret = QUEUE_OK;
    struct queue_body* read_q = get_read_queue(queue->header, queue->mode);
    
    // Check if packet is available - if not, exit immediately
    int sem_err = sem_trywait(&read_q->sem_packets);
    if(sem_err < 0)
        return QUEUE_ERETRY;

    pthread_mutex_lock(&read_q->lock);
    ret = _queue_read(queue, buffer, outSize);
    if(ret < 0)     // If error occurred, treat packet as unread
        sem_post(&read_q->sem_packets);
    pthread_mutex_unlock(&read_q->lock);
    return ret;
}

static void* _queue_get_packet_end(struct queue_body* body, struct queue_element* el) {
    if(el->data + el->size < body->data + body->data_size)
        return el->data + el->size;
    else {
        size_t right = (body->data + body->data_size) - (el->data);
        size_t left = el->size - right;
        return body->data + left;
    }
}

static int _queue_write(queue_t* queue, char* buffer, size_t size) {
    struct queue_body* write_q = get_write_queue(queue->header, queue->mode);

    struct queue_element* last = write_q->last;
    struct queue_element* first = write_q->first;
    size_t free_space = 0;

    if(last == NULL || first == NULL)
        if(size > write_q->data_size)
            return QUEUE_EFAULT;
        
    char* last_end = _queue_get_packet_end(write_q, last);

    if(last_end > first->data) {
        // Queue looks as follow
        //    | ----- FIRST --- .... --- LAST ---- |
        // So the free space is from last to end + from start to first
        free_space = (write_q->data + write_q->data_size) - last_end + (first->data - write_q->data);
        if(size + sizeof(struct queue_element) + QUEUE_WRITE_MARGIN > free_space)
            return QUEUE_E2BIG;
    } else {
        // Queue looks as follow
        //    | ----- LAST --- .... --- FIRST ---- |
        // So the free space is from last to first
        free_space = (first->data - sizeof(struct queue_element)) - last_end;
        if(size + sizeof(struct queue_element) + QUEUE_WRITE_MARGIN > free_space)
            return QUEUE_E2BIG;
    }

    last_end = _queue_get_packet_end(write_q, last);
    if(write_q->last == NULL) {
        write_q->last = write_q->data;
    } else {
        size_t right_space_left = (write_q->data + write_q->data_size) - last_end;
        if(right_space_left > sizeof(struct queue_element) + 5) {
            // Case 1: Enough space for queue_element header
            struct queue_element* new_el = last_end;
            write_q->last->next = new_el;
            write_q->last = new_el;
        } else {
            // Case 2: NOT enough space for queue_element header
            struct queue_element* new_el = write_q->data;
            write_q->last->next = new_el;
            write_q->last = new_el;
        }
        
    }

    write_q->last->next = NULL;
    write_q->last->size = size;

    // TODO: Double memcpy
    size_t copy_size_a = (write_q->data + write_q->data_size) - write_q->last->data;
    if(copy_size_a > size)
        copy_size_a = size;
    ssize_t copy_size_b = write_q->last->size - copy_size_a;
    assert(copy_size_a + copy_size_b == size);

    memcpy(write_q->last->data, buffer, copy_size_a);
    if(copy_size_b)
        memcpy(write_q->data, buffer + copy_size_a, copy_size_b);

    if(write_q->first == NULL)
        write_q->first = write_q->last;
    return QUEUE_OK;
}


int queue_sync_write(queue_t* queue, char* buffer, size_t size) {
    int ret = QUEUE_OK;
    struct queue_body* write_q = get_write_queue(queue->header, queue->mode);

    do {
        pthread_mutex_lock(&write_q->lock);
        ret = _queue_write(queue, buffer, size);
        pthread_mutex_unlock(&write_q->lock);

        if(ret == QUEUE_E2BIG)
            usleep(1000);
    } while(ret == QUEUE_E2BIG);

    if(ret > 0)
        sem_post(&write_q->sem_packets);
    return ret;
}


int queue_async_write(queue_t* queue, char* buffer, size_t size) {
    int ret = QUEUE_OK;
    struct queue_body* write_q = get_write_queue(queue->header, queue->mode);

    pthread_mutex_lock(&write_q->lock);
    ret = _queue_write(queue, buffer, size);
    pthread_mutex_unlock(&write_q->lock);

    if(ret > 0)
        sem_post(&write_q->sem_packets);
    return ret;
}
