CFLAGS=-Wall -Werror -Iinclude -g3 -ggdb3
LDFLAGS=-lev -lpthread
CC=gcc ${CFLAGS} ${LDFLAGS}

all: clean network test

network:
	$(CC) -o evn-test.o -c evn.c

test:
	$(CC) -o evn-test evn-test.c evn.c

clean:
	@rm evn-test evn.o -rf
