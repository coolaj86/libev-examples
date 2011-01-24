#include <stdlib.h> // malloc
#include <string.h> // memcpy

//#include <stdio.h>

#include "inbuf.h"

static inline void* bufcpy(void* dst, void* src, size_t size)
{
  char* d = (char*) dst;
  char* s = (char*) src;
  size_t i;
  for (i = 0; i < size; i += 1)
  {
    *d = *s;
    d += 1;
    s += 1;
  }
  return dst;
}

evn_inbuf* evn_inbuf_create(int size)
{
  evn_inbuf* buf = malloc(sizeof (evn_inbuf)); 
  evn_inbuf_init(buf, size);
  return buf;
}

void evn_inbuf_destroy(evn_inbuf* buf)
{
  free(buf->start);
  free(buf);
}

int evn_inbuf_init(evn_inbuf* buf, int size)
{
  void * new_data;

  size *= 2;
  new_data = malloc(size);

  buf->start = new_data;
  buf->end = new_data += size;
  buf->bottom = new_data;
  buf->top = new_data;

  buf->size = 0;

  // TODO don't assume that malloc worked
  return 0;
}

int evn_inbuf_peek(evn_inbuf* buf, void* data, int size)
{
  //void* new_data;

  if (buf->size < size)
  {
    return -1;
  }

  //new_data = malloc(size);
  memcpy(data, buf->bottom, size);

  //return new_data;
  return 0;
}

void evn_inbuf_toss(evn_inbuf* buf, int size)
{
  if (buf->bottom + size >= buf->top)
  {
    buf->bottom = buf->start;
    buf->top = buf->start;
    buf->size = 0;
    return;
  }

  buf->bottom += size;
  buf->size -= size;
}

int evn_inbuf_add(evn_inbuf* buf, void* data, int size)
{
  size_t capacity = buf->end - buf->start;
  size_t used = buf->top - buf->bottom;
  size_t leading = buf->bottom - buf->start;
  size_t trailing = buf->end - buf->top;
  void* new_data;

  buf->size = used + size;

  if (size <= trailing)
  {
    // use the 
    buf->top = memcpy(buf->top, data, size) + size;
  }
  else if (size <= (leading + trailing) && leading >= (capacity / 2))
  {
    // since memmove() might use a terciary buffer
    // it's faster to alloc new
    // in cases where the buffer is ~100 chars, a loop would prove faster
    new_data = malloc(capacity);
    memcpy(new_data, buf->bottom, used);
    memcpy(new_data + used, data, size);
    free(buf->start);

    buf->start = new_data;
    buf->end = new_data + capacity;
    buf->bottom = new_data;
    buf->top = new_data + used + size;
  }
  else
  {
    do {
      trailing += capacity;
      capacity *= 2;
    } while (size > trailing);

    new_data = realloc(buf->start, capacity);
    // TODO if new_data == buf->start
    buf->start = new_data;
    buf->end = new_data + capacity;
    buf->bottom = new_data + leading;
    buf->top = new_data + used;

    buf->top = memcpy(buf->top, data, size) + size;
  }

  // TODO don't assume that malloc worked
  return 0;
}
