CFLAGS=-Wall -Werror -Iinclude -g3 -ggdb3
LDFLAGS=-lev -lpthread -g3 -ggdb3
CC=gcc ${CFLAGS} ${LDFLAGS}

all: clean server remote test

server:
	$(CC) -o dummyd evn.c dummy-daemon.c dummy-worker.c dummy-worker-thread.c dummy-settings.c rand.c

remote:
	$(CC) -o dummy-rc evn.c dummy-rc.c dummy-settings.c -Iinclude -lev

test:
	$(CC) -o rand-test rand.c rand-test.c -Iinclude
	$(CC) -o dummy-worker-test dummy-worker-test.c dummy-worker.c rand.c -Iinclude

clean:
	@rm dummyd dummy-rc rand-test dummy-worker-test -rf
