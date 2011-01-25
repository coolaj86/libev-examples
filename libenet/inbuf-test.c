#include <stdio.h>

#include "inbuf.h"
#include "evn-assert.c"
#include "string.h"

int main(int argc, char* argv[])
{
  evn_inbuf* buf;
  const char* str_const = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // 36 + 1
  char* str = malloc(strlen(str_const) + 1);
  memcpy(str, str_const, strlen(str_const) + 1);

  buf = evn_inbuf_create(2);
  assert(4 == buf->end - buf->start);
  assert(0 == buf->top - buf->bottom);
  assert(0 == buf->size);

  evn_inbuf_add(buf, "ab", 2);
  assert(4 == buf->end - buf->start);
  assert(2 == buf->top - buf->bottom);
  assert(2 == buf->size);

  evn_inbuf_add(buf, "cd", 2);
  assert(4 == buf->end - buf->start);
  assert(4 == buf->top - buf->bottom);
  assert(4 == buf->size);

  evn_inbuf_add(buf, "efg", 3); // ignore \0
  assert(8 == buf->end - buf->start);
  assert(7 == buf->top - buf->bottom);
  assert(7 == buf->size);

  assert(-1 == evn_inbuf_peek(buf, str, 8));

  evn_inbuf_peek(buf, str, 7);
  printf("abcdefg789... ==\n%s\n", str); // abcdefg789...\0

  evn_inbuf_toss(buf, 4);
  assert(8 == buf->end - buf->start);
  assert(3 == buf->top - buf->bottom);
  assert(3 == buf->size);

  evn_inbuf_add(buf, "hi", 3); // include \0
  assert(8 == buf->end - buf->start);
  assert(6 == buf->top - buf->bottom);
  assert(6 == buf->size);

  printf("efghi ==\n%s\n", (char*) buf->bottom); // efghi\0

  evn_inbuf_destroy(buf);

  return 0;
}
