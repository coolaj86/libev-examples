// TODO variant with compile-time sizes

#include <stdlib.h> // malloc, calloc, realloc
#include <string.h> // memcpy

#include "evn-buffer-list.h"

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
  buffer->position = data;
  buffer->used = 0;
  buffer->capacity = size;
  buffer->free = size;

  return buffer;
}

// Destory a buffer created with `evn_buffer_create<_xyz_>(int size)`
void evn_buffer_destroy(evn_buffer* buffer)
{
  free(buffer->data);
  free(buffer);
}

int evn_buffer_init(evn_buffer* buffer, int size)
{
  void* data = malloc(size * sizeof(size_t));
  if (NULL == data)
  {
    return -1;
  }
  buffer->data = data;
  buffer->position = data;
  buffer->used = 0;
  buffer->capacity = size;
  buffer->free = size;

  return 0;
}

inline evn_buffer* evn_buffer_create_copy(void* data, int size)
{
  evn_buffer* buffer = evn_buffer_create(size);
  memcpy(buffer->data, data, size);
  buffer->position = data + size;
  buffer->used = size;
  buffer->capacity = size;
  buffer->free = 0;

  return buffer;
}

inline evn_buffer* evn_buffer_create_as(void* data, int size, int index)
{
  evn_buffer* buffer = malloc(sizeof(evn_buffer));
  buffer->data = data;
  buffer->position = data + index;
  buffer->used = index;
  buffer->capacity = size;
  buffer->free = size - index;

  return buffer;
}

// copies data into existing buffer and
// returns the number of bytes added
int evn_buffer_add(evn_buffer* buffer, void* data, int size)
{
  int free_space = buffer->capacity - buffer->used;
  int bytes_to_add;
  if (free_space >= size)
  {
    bytes_to_add = size;
  }
  else
  {
    bytes_to_add = free_space;
  }
  memcpy(buffer->position, data, bytes_to_add);
  buffer->position += bytes_to_add;

  buffer->free -= bytes_to_add;
  buffer->used += bytes_to_add;

  return bytes_to_add;
}

// creates bufferlist
evn_bufferlist* evn_bufferlist_create(int min_block_size, int min_slice_count)
{
  if (min_block_size < 1)
  {
    min_block_size = 4096;
  }
  if (min_slice_count < 1)
  {
    min_block_size = 128;
  }
  evn_bufferlist* bufferlist = (evn_bufferlist*) calloc(1, sizeof(evn_bufferlist));
  bufferlist->block_size = min_block_size;
  bufferlist->length = min_slice_count;
  bufferlist->index = -1;
  bufferlist->used = 0;
  bufferlist->size = 0;

  bufferlist->list = (evn_buffer*) malloc(bufferlist->length * sizeof(evn_buffer));
  bufferlist->current = bufferlist->list - 1;
  return bufferlist;
}

int evn_bufferlist_next_buffer(evn_bufferlist* bufferlist)
{
  bufferlist->index += 1;
  bufferlist->current += 1;
  if (bufferlist->index == bufferlist->length)
  {
    bufferlist->length *= 2;
    bufferlist->list = (evn_buffer*) realloc(bufferlist->list, bufferlist->length * sizeof(evn_buffer));
    if (NULL == bufferlist->list)
    {
      // TODO printf("Out-of-Memory! Start Panicking!");
      return -1;
    }
    bufferlist->current = bufferlist->list + bufferlist->index;
  }
  evn_buffer_init(bufferlist->current, bufferlist->block_size);
  bufferlist->size += bufferlist->block_size;

  return 0;
}

// copies data into underlying buffers
int evn_bufferlist_add(evn_bufferlist* bufferlist, void* data, int size)
{
  int total_bytes_copied = 0;
  int bytes_to_copy = size;
  int bytes_copied;

  while (bufferlist->block_size < size)
  {
    bufferlist->block_size *= 2;
  }

  if (-1 == bufferlist->index)
  {
    evn_bufferlist_next_buffer(bufferlist);
  }

  while (total_bytes_copied != bytes_to_copy)
  {
    bytes_copied = evn_buffer_add(bufferlist->current, data, size);
    if (bytes_copied != size)
    {
      evn_bufferlist_next_buffer(bufferlist);
    }
    size -= bytes_copied;
    data += bytes_copied;
    total_bytes_copied += bytes_copied;
  }
  bufferlist->used += total_bytes_copied;

  // TODO check memory copy
  return 0;
}

evn_buffer* evn_bufferlist_concat(evn_bufferlist* bufferlist)
{
  int i;
  void* data = malloc(bufferlist->used);
  evn_buffer* item = bufferlist->list;
  evn_buffer* buffer;

  for (i = 0; i < bufferlist->length; i += 1)
  {
    memcpy(data, item->data, item->used);
    item += 1;
  }

  buffer = evn_buffer_create_as(data, bufferlist->used, bufferlist->used);
  return buffer;
}

void evn_bufferlist_destroy(evn_bufferlist* bufferlist)
{
  int i;
  evn_buffer* item = bufferlist->list;

  for (i = 0; i < bufferlist->length; i += 1)
  {
    free(item->data);
    item += 1;
  }

  free(bufferlist->list);
  free(bufferlist);
}
/*

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
