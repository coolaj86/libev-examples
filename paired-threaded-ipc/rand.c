#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "rand.h"

inline static void
random_init()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);
  srand((unsigned int) tp.tv_usec + (unsigned int) getpid());
}

inline int
random_get(int max) {
  return random_in_range(0, max);
}

inline int
random_in_range(int min, int max)
{
  static int initialized = 0;

  if (!initialized) {
    random_init();
    initialized = 1;
  }

  return rand() % (max - min + 1) + min;
}

#include <stdio.h>
inline int
random_in_range_percent(int base, float percent)
{
  float percentage;
  int variance;

  percentage = percent / 100.0;
  variance = (int) (percentage * base);
  variance = random_get(variance * 2) - variance;

  return base + variance;
}

