CFLAGS=-Wall -Werror -Iinclude -g3 -ggdb3 -I/opt/local/include -I../include
LDFLAGS=-lev -g3 -ggdb3 -L/opt/local/lib
CC=gcc ${CFLAGS}
SCC=${CC} -fPIC -shared

all: clean enet

enet: static shared buffer inbuf

static:
	$(CC) -o obj/buffer-list.o -c buffer-list.c
	$(CC) -o obj/inbuf.o -c inbuf.c
	$(CC) -o obj/enet.o -c evn.c
	ar rcs lib/libenet.a obj/enet.o obj/inbuf.o obj/buffer-list.o

shared:
	$(SCC) -soname,libenet.so.1 \
    -o lib/libenet.so.1.0.0 \
      evn.c \
      buffer-list.c \
      inbuf.c \
      ${LDFLAGS}

buffer:
	$(CC) -o buffer-list-test buffer-list-test.c buffer-list.c ${LDFLAGS}

inbuf:
	$(CC) -o inbuf-test inbuf-test.c inbuf.c ${LDFLAGS}


clean:
	@rm -rf ./lib/ ./obj/ *.dYSM */*.dYSM
	@mkdir -p ./lib/ ./obj/
