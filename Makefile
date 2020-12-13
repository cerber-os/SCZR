CC=clang
CFLAGS=-Wall -g -Og
LIBS=-lrt -lpthread 

all: utils_queue apps_init

utils_queue: utils/queue.c uapi/queue.h
	$(CC) $(CFLAGS) -I uapi/ -c -o out/utils_queue.o utils/queue.c

apps_init: apps/init/init.c utils_queue
	$(CC) $(CFLAGS) -I uapi/ out/utils_queue.o -o out/apps_init $< $(LIBS)
