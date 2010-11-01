#include <stdio.h>

#include "array_heap.h"

void free_msg(void* msg);

int main(int argc, char* argv[]) {
  array arr;
  char* msg = "Hello World!";
  int i;
  printf("pre-init: length %i, index %i\n", arr.length, arr.index);

  array_init(&arr, 32);
  printf("post-init: length %i, index %i\n", arr.length, arr.index);
  
  for (i = 0; i < 64; i += 1) {
    array_push(&arr, msg);
  }
  printf("post-add: length %i, index %i\n", arr.length, arr.index);

  array_free(&arr, free_msg);
  printf("post-free: length %i, index %i\n", arr.length, arr.index);

  printf("didn't segfault\n");
  return 0;
}

void free_msg(void* msg) {
  // we don't actually need to free anything
  // in this simple example
}
