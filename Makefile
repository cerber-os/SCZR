CC=clang
CFLAGS=-Wall -g -Og
LIBS=-lrt -lpthread 

all: utils_queue utils_fastlz utils_device apps_init \
	apps_hypervisor apps_image_generator apps_image_converter \
	apps_image_validator utils_shared_mem utils_misc
	mkdir -p buildroot_cfg/overlay_fs
	cp out/* buildroot_cfg/overlay_fs
	rm buildroot_cfg/overlay_fs/*.o

utils_queue: utils/queue.c uapi/queue.h utils_shared_mem
	$(CC) $(CFLAGS) -I uapi/ -c -o out/utils_queue.o utils/queue.c

utils_fastlz: utils/fastlz.c uapi/fastlz.h
	$(CC) $(CFLAGS) -I uapi/ -c -o out/utils_fastlz.o utils/fastlz.c

utils_device: utils/device.c uapi/device.h
	$(CC) $(CFLAGS) -I uapi/ -c -o out/utils_device.o utils/device.c

utils_shared_mem: utils/shared_mem.c uapi/shared_mem.h
	$(CC) $(CFLAGS) -I uapi/ -c -o out/shared_mem.o utils/shared_mem.c

utils_misc: utils/misc.c uapi/misc.h
	$(CC) $(CFLAGS) -I uapi/ -c -o out/misc.o utils/misc.c

apps_init: apps/init/init.c utils_queue
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o out/shared_mem.o -o out/second_stage_init $< $(LIBS)

apps_hypervisor: apps/hypervisor/hypervisor.c apps/hypervisor/graphics.c utils_queue
	$(CC) $(CFLAGS) -I uapi/ -c -o out/graphics.o apps/hypervisor/graphics.c
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o out/shared_mem.o out/graphics.o -o out/hypervisor $< $(LIBS) -lX11

apps_image_generator: apps/image_generator/main.c utils_queue utils_shared_mem utils_misc
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o out/shared_mem.o out/misc.o -o out/image_generator $< $(LIBS)

apps_image_converter: apps/image_converter/main.c utils_queue utils_fastlz utils_device utils_misc
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o out/shared_mem.o out/misc.o out/utils_fastlz.o out/utils_device.o -o out/image_converter $< $(LIBS)

apps_image_validator: apps/image_validator/main.c utils_queue utils_shared_mem utils_misc
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o out/shared_mem.o out/misc.o -o out/image_validator $< $(LIBS)

clean:
	rm -rf out/*
