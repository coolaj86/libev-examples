// TODO variant with compile-time sizes

#include <stdlib.h> // malloc, calloc, realloc
#include <string.h> // memcpy

#include "buffer-list.h"

// indices are always at the 'free' position
// if the position is equal to the length, the buffer is full
//

// If `data` is not `NULL`, use existing object
// If `data` is `NULL`, allocate `size` bytes
inline evn_buffer* evn_buffer_create(int size)
{
  /*
  void* data = NULL;
  if (NULL == data)
  {
    data = malloc(size * sizeof(size_t));
  }
  */
  void* data = malloc(size * sizeof(size_t));
  evn_buffer* buffer = malloc(sizeof(evn_buffer));
  // TODO check memory
  buffer->data = data;
  buffer->index = 0;
  buffer->size = size;

  return buffer;
}

inline evn_buffer* evn_buffer_create_copy(void* data, int size)
{
  evn_buffer* buffer = evn_buffer_create(size);
  memcpy(buffer->data, data, size);
  buffer->index = size;
  buffer->size = size;

  return buffer;
}

inline evn_buffer* evn_buffer_create_as(void* data, int size, int index)
{
  evn_buffer* buffer = malloc(sizeof(evn_buffer));
  buffer->data = data;
  buffer->size = size;
  buffer->index = index;

  return buffer;
}

// copies data into existing buffer and
// returns the number of bytes added
int evn_buffer_add(evn_buffer* buffer, void* data, int size)
{
  int free_space = buffer->size - buffer->index;
  int bytes_to_add;
  if (free_space >= size)
  {
    bytes_to_add = size;
  }
  else
  {
    bytes_to_add = free_space;
  }
  memcpy(&((char *)buffer->data)[buffer->index], data, bytes_to_add);
  buffer->index += bytes_to_add;

  return bytes_to_add;
}

// creates bufferlist
evn_bufferlist* evn_bufferlist_create(int min_block_size, int min_slice_count)
{
  evn_bufferlist* bufferlist = (evn_bufferlist*) calloc(1, sizeof(evn_bufferlist));
  bufferlist->block_size = block_size || 4096;
  bufferlist->length = slices || 128;
  bufferlist->index = 0;
  bufferlist->used = 0;
  bufferlist->size = 0;

  bufferlist->list = (evn_buffer*) malloc(bufferlist->length * sizeof(evn_buffer));
  return bufferlist;
}

/*

// copies data into underlying buffers
int evn_bufferlist_add(evn_bufferlist* bufferlist, void* data, int size)
{
  evn_buffer* buffer = bufferlist->list[bufferlist->index];

  while(bufferlist->block_size < size)
  {
    bufferlist->block_size *= 2;
  }

  bufferlist->index
}

// adds the buffer, possibly leaving the previous buffer partially used
int evn_bufferlist_add_buffer(evn_bufferlist* bufferlist, evn_buffer* buffer)
{
  void ** list;
  // Check that there is room in the buffer list for more data
  if (bufferlist->size == bufferlist->index)
  {
    list = realloc(bufferlist->list, sizeof(size_t) * buffrlist->size * 2);
    if (NULL == list)
    {
      evn_error("Out of Memory\n");
      return -1;
    }
    bufferlist->list = list;
    bufferlist->size *= 2;
  }
  bufferlist->size += buffer->index + 1; // in case the buffer isn't full
  bufferlist->index += 1;
  bufferlist->list[bufferlist->index] = buffer;
  return 0;
}
*/
