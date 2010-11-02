typedef struct {
  void* data;
  int index;
  int length;
} array;

int array_init(array* arr, int size);
int array_push(array* arr, void* data);
int array_grow(array* arr, int size);
void array_free(array* arr, void (*free_element)(void*));
