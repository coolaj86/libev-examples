#include <string.h> // strlen
#include <stdio.h> // printf
#include <stdlib.h> // exit

#include "evn-buffer-list.h"
#include "evn-assert.c"

int main (int argc, char* argv[])
{
  char* str = "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 )!@#$%^&*("; // 75 chars // 76 bytes
  char* str_buf2 = malloc(strlen(str));
  int bytes_copied = 0;
  memcpy(str_buf2, str, strlen(str) + 1);
  printf("%s\n%d\n", str_buf2, (int) strlen(str_buf2));


  //
  // Buffer
  //

  // Create a buffer of `size` bytes
  evn_buffer* buffer0 = evn_buffer_create(128);
  assert(NULL != buffer0);
  assert(NULL != buffer0->data);
  assert(128 == buffer0->capacity);
  assert(0 == buffer0->used);

  // Create a buffer of `size` by `memcpy`ing `data`
  evn_buffer* buffer1 = evn_buffer_create_copy(str, 27);
  printf("abcdefghijklmnopqrstuvwxyz\n==\n%s\n", (char*) buffer1->data);
  assert(NULL != buffer1);
  assert(NULL != buffer1->data);
  assert(27 == buffer1->capacity);
  assert(27 == buffer1->used);
  assert(0 == memcmp(buffer1->data, str, 27));
  ((char*)buffer1->data)[26] = '\0';

  // Create a buffer from existing data where `index` is the first unused byte (or `size` if used)
  evn_buffer* buffer2 = evn_buffer_create_as(str_buf2, strlen(str) + 1, 27);
  assert(NULL != buffer2);
  assert(NULL != buffer2->data);
  assert(strlen(str) + 1 == buffer2->capacity);
  assert(27 == buffer2->used);
  assert(0 == memcmp(buffer2->data, str, 27));

  // Copy `size` bytes of `data` to existing buffer
  bytes_copied = evn_buffer_add(buffer2, "0123456789 ", 11); // discards trailing \0
  printf("abcdefghijklmnopqrstuvwxyz 0123456789 ...\n==\n%s\n%d\n", (char*) buffer2->data, bytes_copied);
  assert(11 == bytes_copied);
  assert(strlen(str) + 1 == buffer2->capacity);
  assert(38 == buffer2->used);

  // TODO evn_buffer_add_realloc


  // check that remainder is correct
  bytes_copied = evn_buffer_add(buffer2, str, 40); // should copy 38 bytes, leaving 2 uncopied
  ((char*)buffer2->data)[75] = '\0';
  printf("abcdefghijklmnopqrstuvwxyz 0123456789 abcdef...\n==\n%s\n", (char*) buffer2->data);
  assert(strlen(str) + 1 == buffer2->capacity);
  assert(buffer2->used == buffer2->capacity);
  assert(buffer2->capacity - 38 == bytes_copied);


  //
  // BufferList
  //
  printf("BufferList tests...\n");

  // Create a "smart" buffer
  evn_bufferlist* bufferlist = evn_bufferlist_create(40, 1);
  assert(40 == bufferlist->block_size);
  assert(1 == bufferlist->length);
  assert(0 == bufferlist->used);
  assert(0 == bufferlist->size);

  // Copy data into the buffer 
  int result;
  result = evn_bufferlist_add(bufferlist, &str[60], 15);
  assert(1 == bufferlist->length);
  assert(15 == bufferlist->used);
  assert(40 == bufferlist->size);

  // Copy more data into the buffer than the buffer can hold
  result = evn_bufferlist_add(bufferlist, &str, 75);
  assert(2 == bufferlist->length);
  assert(90 == bufferlist->used);
  assert(120 == bufferlist->size); // 40 + 80

  // Copy all data into one big buffer
  evn_buffer* buffer3 = evn_bufferlist_concat(bufferlist);
  assert(90 == buffer3->used);
  assert(90 == buffer3->capacity);
  assert(0 == buffer3->free);

  //
  // Memory Leak Tests
  //
  evn_buffer_destroy(buffer3);
  evn_buffer_destroy(buffer2);
  evn_buffer_destroy(buffer1);
  evn_buffer_destroy(buffer0);
  evn_bufferlist_destroy(bufferlist);
/*
  printf("bs %d\n", bufferlist->block_size);
  printf("len %d\n", bufferlist->length);
  printf("used %d\n", bufferlist->used);
  printf("size %d\n", bufferlist->size);

// Realloc the current buffer and then add a buffer after it
int evn_bufferlist_add_buffer(evn_bufferlist* bufferlist, evn_buffer* buffer);

  evn_buffer* buffer2 = evn_buffer_create_using((void*) data, 2048);

  evn_bufferlist* bufferlist = evn_bufferlist_create(1,1);

  assert(strlen(str) == bufferlist->block_size)
  evn_bufferlist_add_copy(bufferlist, str, strlen(str));
  evn_bufferlist_add_copy(bufferlist, str, strlen(str));

  evn_bufferlist_add_buffer(bufferlist, buffer1);
  assert(2048 == bufferlist->block_size)
  evn_bufferlist_add_copy(bufferlist, str, strlen(str));
  evn_bufferlist_add_copy(bufferlist, str, strlen(str));
  evn_bufferlist_add_copy(bufferlist, str, strlen(str));

  evn_bufferlist_add_buffer(bufferlist, buffer2);
  evn_bufferlist_add_copy(bufferlist, str, strlen(str));
*/
  return 0;
}
