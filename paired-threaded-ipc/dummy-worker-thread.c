#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dummy-worker-thread.h"
#include "dummy-worker.h"
#include "dummy-settings.h"

//function prototypes
static void update_settings(EV_P_ ev_async *w, int revents);
static void process_data(EV_P_ ev_async *w, int revents);
static void cleanup(EV_P_ ev_async *w, int revents);

//the pointer the the control struct in the thread that we
static struct DUMMY_WORKER_THREAD_CONTROL* thread_control;

//our own copies of the settings structs that we give to the DUMMY_WORKER so it doesn't have to worry about mutex locks.
static DUMMY_SETTINGS   dummy_settings;

void* dummy_worker_thread(void* control_struct)
{
  thread_control = (struct DUMMY_WORKER_THREAD_CONTROL*)control_struct;

  pthread_mutex_lock(&(thread_control->settings_lock));
  memcpy(&dummy_settings, thread_control->dummy_settings, sizeof dummy_settings);
  pthread_mutex_unlock(&(thread_control->settings_lock));

  // Initialize the codec
  if(EXIT_SUCCESS != worker_init(&dummy_settings)) {
    fprintf(stderr, "failed init DUMMY_WORKER algorithm\n");
    exit(EXIT_FAILURE);
  }
  
  //now set the watchers for the appropriate action.
  ev_async_init (&(thread_control->update_settings), &update_settings);
  ev_async_start(thread_control->EV_A, &(thread_control->update_settings));

  ev_async_init (&(thread_control->process_data), &process_data);
  ev_async_start(thread_control->EV_A, &(thread_control->process_data));

  ev_async_init (&(thread_control->cleanup), &cleanup);
  ev_async_start(thread_control->EV_A, &(thread_control->cleanup));

  ev_loop(thread_control->EV_A, 0);

  fprintf(stderr, "the event loop for the DUMMY_WORKER thread exitted\n");
  return NULL;
}

static void update_settings(EV_P_ ev_async *w, int revents)
{
  pthread_mutex_lock(&(thread_control->settings_lock));
  memcpy(&dummy_settings, thread_control->dummy_settings, sizeof dummy_settings);
  pthread_mutex_unlock(&(thread_control->settings_lock));

  worker_set(&dummy_settings);
}

static void process_data(EV_P_ ev_async *w, int revents)
{
  static char current_file[32];

  pthread_mutex_lock(&(thread_control->buffer_lock));
  strncpy(current_file, thread_control->buffer[thread_control->buffer_head], sizeof current_file);
  thread_control->buffer_head = (thread_control->buffer_head + 1) % BUFFER_SIZE;
  thread_control->buffer_count -= 1;
  pthread_mutex_unlock(&(thread_control->buffer_lock));

  worker_run(current_file);
}

static void cleanup(EV_P_ ev_async *w, int revents)
{
  worker_clean();
  ev_unloop(EV_A_ EVUNLOOP_ALL);
}
