#include <pthread.h> // pthread_t, pthread_create
#include <string.h> // strerror
#include <stdio.h> // fprintf puts
#include <unistd.h> // sleep

void* do_stuff_thread(void* args)
{
  while(1)
  {
    puts("I'm doing stuff");
    sleep(1);
  }
  return NULL;
}

void* do_things_thread(void* args)
{
  while(1)
  {
    puts("I'm doing things");
    sleep(1);
  }
  return NULL;
}

int main (int argc, char* argv[])
{
  int thread_status;
  pthread_attr_t attr;
  pthread_t do_stuff_pthread;
  pthread_t do_things_pthread;
  void* exit_status;

  puts("Hello World!");

  // Redundant defaults, NULL would do the same thing
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // redundant default

  thread_status = pthread_create(&do_stuff_pthread, &attr, do_stuff_thread, NULL);
  if (0 != thread_status)
  {
    fprintf(stderr, "thread creation failed with errno %d (%s)\n", thread_status, strerror(thread_status));
  }

  thread_status = pthread_create(&do_things_pthread, &attr, do_things_thread, NULL);
  if (0 != thread_status)
  {
    fprintf(stderr, "thread creation failed with errno %d (%s)\n", thread_status, strerror(thread_status));
  }

  pthread_attr_destroy(&attr);


  // If the threads are not joined, main will exit
  // and subsequently kill them, possibly never having been run
  pthread_join(do_stuff_pthread, &exit_status);
  pthread_join(do_things_pthread, &exit_status);
  
  puts("Good Bye!");
  return 0;
}
