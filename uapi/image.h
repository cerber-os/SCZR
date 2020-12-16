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

void* create_shared_memory(size_t size) {
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_SHARED | MAP_ANONYMOUS;

  // The remaining parameters to `mmap()` are not important for this use case,
  // but the manpage for `mmap` explains their purpose.
  return mmap(NULL, size, protection, visibility, -1, 0);
}

#endif