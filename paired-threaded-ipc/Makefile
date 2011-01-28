CFLAGS=-Wall -Werror -Iinclude -g3 -ggdb3 -I/opt/local/include -I../include
LDFLAGS=-lev -levnet -lpthread -g3 -ggdb3 -L/opt/local/lib -L./lib
# on OS X include -largp
# LDFLAGS+= -largp
CC=gcc ${CFLAGS} ${LDFLAGS}

all: enet clean server remote test

enet:
	@mkdir -p ./lib/
	make -C ../libevnet
	cp -a ../libevnet/lib/libevnet.a ./lib/
	cp -a ../libevnet/lib/libevnet.so* ./lib/

server:
	$(CC) -o dummyd dummy-daemon.c dummy-worker.c dummy-worker-thread.c dummy-settings.c rand.c -levnet -L./lib

remote:
	$(CC) -o dummy-rc dummy-rc.c dummy-settings.c -levnet -L./lib

test:
	$(CC) -o rand-test rand.c rand-test.c
	$(CC) -o dummy-worker-test dummy-worker-test.c dummy-worker.c rand.c

clean:
	@rm -rf *.dSYM dummyd dummy-rc *-test
