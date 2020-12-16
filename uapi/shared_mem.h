#include <stdlib.h>

/*
 * Create new shared memory file
 *  @name - unique name of memory file
 *  @size - size in bytes of shared memory 
 *  @return - pointer to shared memory
 * It is caller duty to release allocated memory using munmap !!!
 */
void* create_shared_memory(char* name, size_t size);

/*
 * Get pointer to created shared memory by unique name
 *  @name - unique name of memory file
 *  @size - size in bytes of shared memory 
 *  @return - pointer to shared memory
 * It is caller duty to release allocated memory using munmap !!!
 */
void* get_shared_memory(char* name, size_t size);

/*
 * Remove shared memory region - it won't be accessible anymore
 *  @name - unique name of memory file to delete
 * It is caller duty to release previously allocated memory using munmap !!!
 */
void delete_shared_memory(char* name);
