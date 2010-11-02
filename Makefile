CC_OPTS = -Wall -Werror -lev -ggdb3

all: array_heap.o unix-echo-server unix-echo-client udp-echo

clean:
	rm -f *.o array-test unix-echo unix-echo-client udp-echo

array-test: array_heap.o array_test.c
	$(CC) $(CC_OPTS) -o array-test array_test.c array_heap.o

array_heap.o: array_heap.c array_heap.h
	$(CC) $(CC_OPTS) -o array_heap.o -c array_heap.c

udp-echo-server: udp-echo-server.c
	$(CC) $(CC_OPTS) -o udp-echo udp-echo.c

unix-echo: unix-echo-server.c array_heap.o
	$(CC) $(CC_OPTS) -o $@ $< array_heap.o

unix-echo-client: unix-echo-client.c
	$(CC) $(CC_OPTS) -o $@ $<
