CC=clang
CFLAGS=-Wall -g -Og
LIBS=-lrt -lpthread 

all: utils_queue utils_fastlz utils_device apps_init apps_hypervisor apps_image_generator apps_image_converter 

utils_queue: utils/queue.c uapi/queue.h
	$(CC) $(CFLAGS) -I uapi/ -c -o out/utils_queue.o utils/queue.c

utils_fastlz: utils/fastlz.c uapi/fastlz.h
	$(CC) $(CFLAGS) -I uapi/ -c -o out/utils_fastlz.o utils/fastlz.c

utils_device: utils/device.c uapi/device.h
	$(CC) $(CFLAGS) -I uapi/ -c -o out/utils_device.o utils/device.c

apps_init: apps/init/init.c utils_queue
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o -o out/init $< $(LIBS)

apps_hypervisor: apps/hypervisor/hypervisor.c utils_queue
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o -o out/hypervisor $< $(LIBS)

apps_image_generator: apps/image_generator/main.c utils_queue
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o -o out/image_generator $< $(LIBS)

apps_image_converter: apps/image_converter/main.c utils_queue utils_fastlz utils_device
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o out/utils_fastlz.o out/utils_device.o -o out/image_converter $< $(LIBS)

clean:
	rm -rf out/*
