CFLAGS=-Wall -Werror -Iinclude -g3 -ggdb3 -I/opt/local/include -I../include
LDFLAGS=-lev -g3 -ggdb3 -L/opt/local/lib
CC=gcc ${CFLAGS}
SCC=${CC} -fPIC -shared

all: clean enet example

enet: static shared buffer inbuf

example: static
	$(CC) -o echo-client echo-client.c ${LDFLAGS} -levnet -L./lib
	$(CC) -o echo-server echo-server.c ${LDFLAGS} -levnet -L./lib

static:
	$(CC) -o obj/buffer-list.o -c buffer-list.c
	$(CC) -o obj/inbuf.o -c inbuf.c
	$(CC) -o obj/enet.o -c evn.c
	ar rcs lib/libevnet.a obj/enet.o obj/inbuf.o obj/buffer-list.o

shared:
	$(SCC) -Wl,-soname,libevnet.so.1 \
    -o lib/libevnet.so.1.0.0 \
      evn.c \
      buffer-list.c \
      inbuf.c \
      ${LDFLAGS}

buffer:
	$(CC) -o buffer-list-test buffer-list-test.c buffer-list.c ${LDFLAGS}

inbuf:
	$(CC) -o inbuf-test inbuf-test.c inbuf.c ${LDFLAGS}


clean:
	@rm -rf ./lib/ ./obj/ *.dYSM */*.dYSM *-test echo-client echo-server
	@mkdir -p ./lib/ ./obj/
