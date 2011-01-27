#ifndef DUMMY_WORKER_THREAD_STRUCT
#define DUMMY_WORKER_THREAD_STRUCT

#include <ev.h>
#include <pthread.h>

#include "dummy-settings.h"

// this is the struct containing all the variables that the main thread needs to communicate with the DUMMY_WORKER thread
// we do this so we can easily pass the info into the secondary thread without using any convoluting externs
#define BUFFER_SIZE 6

struct DUMMY_WORKER_THREAD_CONTROL {
  //the buffer and the mutex lock that prevent multiple read/writes at the same time.
  pthread_mutex_t buffer_lock;
  char buffer[BUFFER_SIZE][32]; // filename
  int buffer_count;
  int buffer_head;

  //pointers to settings structs and the mutex lock to protect them
  pthread_mutex_t settings_lock;
  DUMMY_SETTINGS*   dummy_settings;

  //the async handlers libev provides to communicate between threads.
  ev_async update_settings;
  ev_async process_data;
  ev_async cleanup;
  
  // the actual event loop used to talk across threads.
  EV_P;
};

void* dummy_worker_thread(void* control_struct);
#endif
