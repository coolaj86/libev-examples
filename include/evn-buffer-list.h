#ifndef EVN_BUFFER_LIST_H_
#define EVN_BUFFER_LIST_H_

#include <stdio.h> // printf

#define evn_error(...) printf("[EVN] " __VA_ARGS__)

typedef struct {
  void* data;
  void* position;

  int free;
  int used; // -> index
  int capacity; // -> size
} evn_buffer;

typedef struct {
  evn_buffer* list;
  evn_buffer* current;

  int block_size;
  int index;
  int length;
  int used;
  int size;
} evn_bufferlist;


// Create a buffer of `size` bytes
evn_buffer* evn_buffer_create(int size);
// Create a buffer of `size` by `memcpy`ing `data`
evn_buffer* evn_buffer_create_copy(void* data, int size);
// Create a buffer from existing data where `index` is the first unused byte (or `size` if used)
evn_buffer* evn_buffer_create_as(void* data, int size, int index);

// Copy `size` bytes of `data` to existing buffer
int evn_buffer_add(evn_buffer* buffer, void* data, int size);

// Free all memory associated with the buffer
void evn_buffer_destroy(evn_buffer* buffer);

// Create a "smart" buffer
evn_bufferlist* evn_bufferlist_create(int min_block_size, int slices);
// Copy data into the buffer
int evn_bufferlist_add(evn_bufferlist* bufferlist, void* data, int size);
// Copy all data into a single buffer
evn_buffer* evn_bufferlist_concat(evn_bufferlist* bufferlist);
// Realloc the current buffer and then add a buffer after it
//int evn_bufferlist_add_buffer(evn_bufferlist* bufferlist, evn_buffer* buffer);

// Free all memory associated with the bufferlist and the buffers it contains
void evn_bufferlist_destroy(evn_bufferlist* bufferlist);

#endif
