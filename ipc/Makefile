LD_FLAGS = -lev
CC_FLAGS = -Wall -Werror -I../include
CC += $(CC_FLAGS) $(LD_FLAGS)

all:
	$(CC) -o unix-socket-ipc-remote-control unix-socket-ipc-remote-control.c
	$(CC) -o unix-socket-ipc-daemon unix-socket-ipc-daemon.c ../obj/array-heap.o
