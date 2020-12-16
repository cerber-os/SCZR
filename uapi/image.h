#ifndef IMAGE_H
#define IMAGE_H

struct pixel {
    char red;
    char green;
    char blue;
};

struct image
{
    int width;
    int height;
    struct pixel pixel_info[0];
};

struct packet
{
    struct timespec start;
    struct timespec stop;
    char data[0];
};

#endif