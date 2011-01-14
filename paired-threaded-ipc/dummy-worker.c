#include <unistd.h> // usleep
#include <stdio.h> // perror
#include <string.h> // memcpy


#include "dummy-worker.h"
#include "rand.h"

static DUMMY_SETTINGS settings;

static void msleep(int time) {
  if (0 != usleep((useconds_t) (time * 1000)))
  {
    perror("couldn't usleep (possibly caught a signal)");
  }
}

int
worker_init(DUMMY_SETTINGS* new_settings)
{
  worker_set(new_settings);
  msleep(settings.init_time);
  return 0;
}

void
worker_run(void* data)
{
  int proc_time;

  if (settings.variance)
  {
    proc_time = random_in_range_percent(settings.delay, (float) settings.variance);
  }
  else
  {
    proc_time = settings.delay;
  }

  msleep(proc_time);
}

void
worker_clean()
{
  msleep(settings.init_time);
}

void
worker_set(DUMMY_SETTINGS* new_settings)
{
  memcpy((void*) &settings, (void*) new_settings, (size_t) sizeof(DUMMY_SETTINGS));
}
