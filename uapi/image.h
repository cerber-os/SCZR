#ifndef IMAGE_H
#define IMAGE_H

struct pixel {
    char red;
    char green;
    char blue;
} __attribute__((packed));

#define IMAGE_SIZE(W, H)    ((W) * (H) * sizeof(struct pixel))

#endif