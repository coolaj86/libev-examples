#include <stdio.h>
#include <stdlib.h>

void assert(int truth)
{
  static int count = 0;
  if (!truth)
  {
    printf("failed assertion %d\n", count);
    exit(EXIT_FAILURE);
  }
  else
  {
    printf("Pass\n");
  }
  count += 1;
}

