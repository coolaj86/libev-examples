#include <stdio.h> // printf
#include <sys/time.h> // gettimeofday

#include "bool.h"
#include "loop.h"
#include "dummy-worker.h"

static DUMMY_SETTINGS settings;

void
time_delta_print(char* prefix)
{
  static int count;
  static struct timeval last_timep = {};
  struct timeval timep;
  int delta;

  if (!last_timep.tv_usec)
  {
    gettimeofday(&last_timep, NULL);
    return;
  }

  gettimeofday(&timep, NULL);

  delta = (int) (timep.tv_usec - last_timep.tv_usec);
  if (delta < 0)
  {
    delta += 1000000;
  }
  printf("%3d %s: %.3f ms\n", count, prefix, delta / 1000.0);

  last_timep = timep;
  count += 1;
}


void
work()
{
  static char data[4096 * 1024];
  int i = 0;

  loop {
    if (i >= 10)
    {
      return;
    }
    worker_run(&data);
    time_delta_print("run");
    i += 1;
  }
}


int
main (int argc, char* argv[])
{
  settings.init_time = 10;
  settings.delay = 128;
  settings.variance = 15;

  time_delta_print("begin");

  worker_init(&settings);
  time_delta_print("init");

  worker_set(&settings);
  time_delta_print("set");

  work();

  worker_clean();
  time_delta_print("clean");

  return 0;
}
