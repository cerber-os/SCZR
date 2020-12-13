/*
 * HAL of remote communication device
 */

#include <stdlib.h>

typedef struct ir_device {
    // TODO: Fill according to needs
    int fd;
} ir_device;

/*
 * Open remote device
 *  @name: path to device
 *  @return: pointer to structure representing the device
 *           or NULL on error
 */
ir_device* ir_open(char* name);

/*
 * Read upto `size` bytes from the device to `buffer`
 *  @dev: pointer to structure obtained by call to ir_open
 *  @buffer: pointer to buffer of at least `size` bytes
 *  @size: maximum number of bytes to read
 *  @return: number of bytes read or -1 on error (see errno)
 */
int ir_read(ir_device* dev, char* buffer, size_t size);

/*
 * Write upto `size` bytes from `buffer` to the device
 *  @dev: pointer to structure obtained by call to ir_open
 *  @buffer: pointer to buffer of at least `size` bytes
 *  @size: maximum number of bytes to write
 *  @return: number of bytes written or -1 on error (see errno)
 */
int ir_write(ir_device* dev, char* buffer, size_t size);

/*
 * Close remote device
 *  @name: pointer to device struct
 */
void ir_close(ir_device* dev);
