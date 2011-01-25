CFLAGS=-Wall -Werror -Iinclude -g3 -ggdb3 -I/opt/local/include
LDFLAGS=-lev -lpthread -g3 -ggdb3 -L/opt/local/lib -largp
CC=gcc ${CFLAGS} ${LDFLAGS}

all: clean server remote test buffer

server:
	$(CC) -o dummyd evn.c dummy-daemon.c dummy-worker.c dummy-worker-thread.c dummy-settings.c rand.c buffer-list.c inbuf.c

remote:
	$(CC) -o dummy-rc evn.c dummy-rc.c dummy-settings.c buffer-list.c inbuf.c

buffer:
	$(CC) -o buffer-list-test buffer-list-test.c buffer-list.c

inbuf:
	$(CC) -o inbuf-test inbuf-test.c inbuf.c

test:
	$(CC) -o rand-test rand.c rand-test.c
	$(CC) -o dummy-worker-test dummy-worker-test.c dummy-worker.c rand.c

clean:
	@rm -rf *.dSYM dummyd dummy-rc *-test
