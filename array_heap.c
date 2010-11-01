#include <stdlib.h>
#include <stdio.h>

#include "array_heap.h"

int array_init(array* arr, int size) {
  arr->data = realloc(NULL, sizeof(void*) * size);
  if (0 == (int)arr->data) {
    return -1;
  }
  arr->length = size;
  arr->index = 0;
  return 0;
}

int array_push(array* arr, void* data) {
  ((int*)arr->data)[arr->index] = (int)data;
  arr->index += 1;
  if (arr->index >= arr->length) {
    return array_grow(arr, arr->length * 2);
  }
  return 0;
}

int array_grow(array* arr, int size) {
  if (size <= arr->length) {
    return -1;
  }
  arr->data = realloc(arr->data, sizeof(void*) * size);
  if (-1 == (int)arr->data) {
    return -1;
  }
  arr->length = size;
  return 0;
}

void array_free(array* arr, void (*free_element)(void*)) {
  int i;
  for (i = 0; i < arr->index; i += 1) {
    free_element((void*)((int*)arr->data)[i]);
  }
  free(arr->data);
  arr->index = -1;
  arr->length = 0;
  arr->data = NULL;
}
