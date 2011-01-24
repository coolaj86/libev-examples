#ifndef _EVN_INBUF_H_
#define _EVN_INBUF_H_

// This buffer is optimized for situations where the
// drain is roughly equal to the increase.
// (i.e. the user listens to the `drain` event before
// stacking on another full chunk of data


struct evn_inbuf;

typedef struct {
  void* start;
  void* end;
  void* bottom;
  void* top;
  int size;
} evn_inbuf;

/*

Anotomy of evn_inbuf

  start|        |size|        |end
       |        |    |        |
  DATA |oooooooo|xxxx|oooooooo|
                |    |
          bottom|    |top
                |    |
        |leading|    |trailing|
                |    |
                |used|

The buffer will hold up to two chunks.
It is assumed that one chunk should dain at about the same rate that the next chunk comes in.
The buffer will realloc to become twice its size if the incoming data if space is no avaibale.
It will memcpy if there is at least a chunks-worth of space already drained

*/

// Create a buffer about the size you expect the maximum incoming chunk to be
// No need to get crazy, the size will grow on it's own, but it will never shrink
evn_inbuf* evn_inbuf_create(int size);
int evn_inbuf_init(evn_inbuf* buf, int size);
// Add a chunk to the buffer
int evn_inbuf_add(evn_inbuf* buf, void* data, int size);
// Peek at a copy of some amount of data (you malloc the data)
int evn_inbuf_peek(evn_inbuf* buf, void* data, int size);
// Toss some amount of data out
void evn_inbuf_toss(evn_inbuf* buf, int size);
// Free memory associated with this inbuf
void evn_inbuf_destroy(evn_inbuf* buf);

#endif
