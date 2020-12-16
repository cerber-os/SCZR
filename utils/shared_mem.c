#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "shared_mem.h"

void* create_shared_memory(char* name, size_t size) {
    int memfd = shm_open(name, O_CREAT | O_TRUNC | O_RDWR, 0755);
    if(memfd < 0) {
        perror("[-] shared_mem: shm_open failed");
        return NULL;
    }
    
    int ret = ftruncate(memfd, size);
    if(ret < 0) {
        perror("[-] shared_mem: ftruncate failed");
        close(memfd);
        return NULL;
    }

    void* addr = mmap(NULL, 
                    size, 
                    PROT_READ | PROT_WRITE, 
                    MAP_SHARED, 
                    memfd, 0);
    if(addr == MAP_FAILED) {
        perror("[-] shared_mem: mmap failed");
        close(memfd);
        return NULL;
    }

    close(memfd);
    return addr;
}


void* get_shared_memory(char* name, size_t size) {
    int memfd = shm_open(name, O_RDWR, 0755);
    if(memfd < 0) {
        perror("[-] shared_mem(get): shm_open failed");
        return NULL;
    }

    void* addr = mmap(NULL, 
                    size, 
                    PROT_READ, 
                    MAP_SHARED, 
                    memfd, 0);
    if(addr == MAP_FAILED) {
        perror("[-] shared_mem(get): mmap failed");
        close(memfd);
        return NULL;
    }

    close(memfd);
    return addr;
}


void delete_shared_memory(char* name) {
    shm_unlink(name);
}
