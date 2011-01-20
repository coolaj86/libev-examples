#include <string.h> // strlen
#include <stdio.h> // printf
#include <stdlib.h> // exit

#include "buffer-list.h"

void assert(int truth)
{
  static int count = 0;
  if (!truth)
  {
    printf("failed assertion %d\n", count);
    exit(EXIT_FAILURE);
  }
  else
  {
    printf("Pass\n");
  }
  count += 1;
}

int main (int argc, char* argv[])
{
  char* str = "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 )!@#$%^&*("; // 75 chars // 76 bytes
  char* str_buf2 = malloc(strlen(str));
  int bytes_copied = 0;
  memcpy(str_buf2, str, strlen(str) + 1);
  printf("%s\n%d\n", str_buf2, strlen(str_buf2));

  //
  // Buffer
  //

  // Create a buffer of `size` bytes
  evn_buffer* buffer0 = evn_buffer_create(128);
  assert(NULL != buffer0);
  assert(NULL != buffer0->data);
  assert(128 == buffer0->size);
  assert(0 == buffer0->index);

  // Create a buffer of `size` by `memcpy`ing `data`
  evn_buffer* buffer1 = evn_buffer_create_copy(str, 27);
  printf("abcdefghijklmnopqrstuvwxyz\n==\n%s\n", (char*) buffer1->data);
  assert(NULL != buffer1);
  assert(NULL != buffer1->data);
  assert(27 == buffer1->size);
  assert(27 == buffer1->index);
  assert(0 == memcmp(buffer1->data, str, 27));
  ((char*)buffer1->data)[26] = '\0';

  // Create a buffer from existing data where `index` is the first unused byte (or `size` if used)
  evn_buffer* buffer2 = evn_buffer_create_as(str_buf2, strlen(str) + 1, 27);
  assert(NULL != buffer2);
  assert(NULL != buffer2->data);
  assert(strlen(str) + 1 == buffer2->size);
  assert(27 == buffer2->index);
  assert(0 == memcmp(buffer2->data, str, 27));

  // Copy `size` bytes of `data` to existing buffer
  bytes_copied = evn_buffer_add(buffer2, "0123456789 ", 11); // discards trailing \0
  printf("abcdefghijklmnopqrstuvwxyz 0123456789 ...\n==\n%s\n", (char*) buffer2->data);
  assert(11 == bytes_copied);
  assert(strlen(str) + 1 == buffer2->size);
  assert(38 == buffer2->index);

  // TODO evn_buffer_add_realloc


  // check that remainder is correct
  bytes_copied = evn_buffer_add(buffer2, str, 40); // should copy 38 bytes, leaving 2 uncopied
  ((char*)buffer2->data)[75] = '\0';
  printf("abcdefghijklmnopqrstuvwxyz 0123456789 abcdef...\n==\n%s\n", (char*) buffer2->data);
  assert(strlen(str) + 1 == buffer2->size);
  assert(buffer2->index == buffer2->size);
  assert(buffer2->size - 38 == bytes_copied);

  //
  // BufferList
  //

  // Create a "smart" buffer
  evn_bufferlist* evn_bufferlist_create(int min_block_size, int min_slices);
/*

// Copy data into the buffer 
int evn_bufferlist_add_data(evn_bufferlist* bufferlist, void* data, int size);
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
