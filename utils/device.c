#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "device.h"


ir_device* ir_open(char* name)
{
    struct termios term;
    int fd = open(name, O_RDWR);
    if(fd < 0) {
        perror("[-] device: failed to open file");
        return NULL;
    }

    tcgetattr(fd, &term);

    cfsetispeed(&term, IR_BAUD_RATE);
    cfsetospeed(&term, IR_BAUD_RATE);
    term.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    term.c_cflag |= CS8 | CLOCAL;

    term.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                           | INLCR | IGNCR | ICRNL | IXON);
    term.c_oflag &= ~OPOST;
    term.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    term.c_cflag &= ~(CSIZE | PARENB);
    term.c_cflag |= CS8;

    tcsetattr(fd, TCSANOW, &term);
    tcflush(fd, TCIFLUSH);
    tcflush(fd, TCOFLUSH);

    ir_device* dev = malloc(sizeof(ir_device));
    if(dev == NULL) {
        printf("[-] device: Failed to allocate memory for ir_device\n");
        return NULL;
    }

    dev->fd = fd;
    return dev;
}


int ir_read(ir_device* dev, void* buffer, size_t size)
{
    if(dev == NULL) {
        printf("[i] device: using STUB ir_read\n");
        memset(buffer, 0, size);
        return size;
    }
    return read(dev->fd, buffer, size);
}


int ir_write(ir_device* dev, void* buffer, size_t size)
{
    if(dev == NULL) {
        printf("[i] device: using STUB ir_write\n");
        return size;
    }
    
    int ret = write(dev->fd, buffer, size);
    usleep(100);
    // tcflush(dev->fd,TCIOFLUSH);
    return ret;
}


void ir_close(ir_device* dev)
{
    if(dev == NULL)
        return;
    close(dev->fd);
    free(dev);
}