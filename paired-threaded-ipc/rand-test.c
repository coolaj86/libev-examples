#include <stdio.h>

#include "rand.h"

const int LOW = 1;
const int HIGH = 6;

inline int
roll()
{
  //return random_in_range(LOW, HIGH);
                                //Base, Percent
  return random_in_range_percent(128, 15);
}

int main()
{
  printf("You roll [%d] [%d]\n", roll(), roll());
  printf("PC rolls [%d] [%d]\n", roll(), roll());

  return 0;
}
