#include <argp.h>           //the command line argument parser
#include <sys/resource.h>   //setpriority
#include <signal.h>         //signals to process data and quit cleanly

#include "dummy-worker-thread.h"

// Standard Stuff
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// Libev
#include <ev.h>
#include <pthread.h>

#include "dummy-settings.h"
#include "bool.h"

#include "evn.h"

#define LOG_PATH "/tmp/"

// to move out
static void dwt_enqueue(char* filename);

// Function Prototypes
void clean_shutdown(EV_P_ int sig);
static void test_process_is_not_blocked(EV_P_ ev_periodic *w, int revents);

// EVN Network Callbacks
static void stream_on_data(EV_P_ struct evn_stream* stream, void* data, int n);
static void stream_on_close(EV_P_ struct evn_stream* stream, bool had_error);
static void server_on_connection(EV_P_ struct evn_server* server, struct evn_stream* stream);

// Help / Docs
const char* argp_program_version = "ACME DummyServe v1.0 (Alphabet Animal)";
const char* argp_program_bug_address = "<bugs@example.com>";

/* Program documentation. */
static char doc[] = "ACME DummyServe -- An abstraction layer between the Dummy Processing Algorithms and Business Logic";

//settings structs
static DUMMY_SETTINGS dummy_settings;
static bool redirect = false;

static pthread_t dummy_worker_pthread;
static struct DUMMY_WORKER_THREAD_CONTROL thread_control;


/* The options we understand. */
static struct argp_option options[] = {
#include "dummy_settings_argp_option.c"
  {"write-logs"        , 'w', 0      , 0,  "redirect stdout and stderr to log files" },
  { 0 }
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
  {
#include "dummy_settings_parse_opt.c"

    case 'w':
      redirect = true;
      break;

    case ARGP_KEY_ARG:
      return ARGP_ERR_UNKNOWN;

    case ARGP_KEY_END:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

// Our argument parser.
static struct argp argp = { options, parse_opt, 0, doc };

void clean_shutdown(EV_P_ int sig) {
  void* exit_status;

  puts("[Daemon] Shutting Down.\n");
  fprintf(stderr, "Received signal %d, shutting down\n", sig);
  // TODO handle signal close(server.fd);
  ev_async_send(thread_control.EV_A, &(thread_control.cleanup));

  //this will block until the dummy_worker_pthread exits, at which point we will continue with out shutdown.
  pthread_join(dummy_worker_pthread, &exit_status);
  ev_loop_destroy (EV_DEFAULT_UC);
  puts("[dummyd] Dummy cleanup finished.");
  exit(EXIT_SUCCESS);
}

void redirect_output()
{
  // these lines are to direct the stdout and stderr to log files we can access even when run as a daemon (after the possible help info is displayed.)
  //open up the files we want to use for out logs
  int new_stderr, new_stdout;
  new_stderr = open(LOG_PATH "dummy_error.log", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  new_stdout = open(LOG_PATH "dummy_debug.log", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  //truncate those files to clear the old data from long since past.
  if (0 != ftruncate(new_stderr, 0))
  {
    perror("could not truncate stderr log");
  }
  if (0 != ftruncate(new_stdout, 0))
  {
    perror("could not truncate stdout log");
  }

  //duplicate the new file descriptors and assign the file descriptors 1 and 2 (stdout and stderr) to the duplicates
  dup2(new_stderr, 2);
  dup2(new_stdout, 1);

  //now that they are duplicated we can close them and let the overhead c stuff worry about closing stdout and stderr later.
  close(new_stderr);
  close(new_stdout);
}

int main (int argc, char* argv[])
{
  // Create our single-loop for this single-thread application
  EV_P;
  pthread_attr_t attr;
  int thread_status;
  struct evn_server* server;
  char socket_address[256];
  EV_A = ev_default_loop(0);


  // Set default options, then parse new arguments to update the settings
  dummy_settings_set_presets(&dummy_settings);
  argp_parse (&argp, argc, argv, 0, 0, &dummy_settings);
  // TODO separate worker settings from daemon settings
  // (i.e. redirect)

  if (true == redirect)
  {
    redirect_output();
  }

  if (0)
  {
    struct ev_periodic every_few_seconds;

    // To be sure that we aren't actually blocking
    ev_periodic_init(&every_few_seconds, test_process_is_not_blocked, 0, 1, 0);
    ev_periodic_start(EV_A_ &every_few_seconds);
  }

  // Set the priority of this whole process higher (requires root)
  setpriority(PRIO_PROCESS, 0, -13); // -15

  // initialize the values of the struct that we will be giving to the new thread
  thread_control.dummy_settings = &dummy_settings;
  thread_control.buffer_head = 0;
  thread_control.buffer_count = 0;
  thread_control.EV_A = ev_loop_new(EVFLAG_AUTO);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // Initialize the thread that will manage the DUMMY_WORKER
  thread_status = pthread_create(&dummy_worker_pthread, &attr, dummy_worker_thread, (void *)(&thread_control));
  if (0 != thread_status)
  {
    fprintf(stderr, "thread creation failed with errno %d (%s)\n", thread_status, strerror(thread_status));
    exit(EXIT_FAILURE);
  }
  pthread_attr_destroy(&attr);


  // Create unix socket in non-blocking fashion
  snprintf(socket_address, sizeof(socket_address), DUMMYD_SOCK, (int)getuid());
  unlink(socket_address);
  server = evn_server_create(EV_A_ server_on_connection);
  server->on_connection = server_on_connection;
  evn_server_listen(server, 0, socket_address);

  // Run our loop, until we recieve the QUIT, TERM or INT signals, or an 'x' over the socket.
  puts("[Daemon] Looping.\n");
  ev_loop(EV_A_ 0);

  // Cleanup if `unloop` is ever called
  clean_shutdown(EV_A_ 0);
  return 0;
}

static void test_process_is_not_blocked(EV_P_ ev_periodic *w, int revents) {
  static int up_time = 0;
  struct stat sb;
  int status1, status2;

  //keep the stdout log file from getting too big and crashing the system after a long time running.
  //we are in big trouble anyway if the stderr file gets that big.
  fstat(1, &sb);
  if(sb.st_size > 1048576)
  {
    status1 = ftruncate(1, 0);
    status2 = lseek(1, 0, SEEK_SET);
    if ( (0 != status1) || (0 != status2))
    {
      perror("truncating stdout file");
    }
    else
    {
      printf("successfully truncated the stdout file\n");
    }
  }

  printf("I'm still alive (%d)\n", up_time);
  up_time += 1;
}

static void dwt_enqueue(char* filename)
{
  int buffer_tail;

  pthread_mutex_lock(&(thread_control.buffer_lock));
  if (thread_control.buffer_count < BUFFER_SIZE)
  {
    buffer_tail = (thread_control.buffer_head + thread_control.buffer_count) % BUFFER_SIZE;
    strncpy(thread_control.buffer[buffer_tail], filename, sizeof thread_control.buffer[buffer_tail]);
    thread_control.buffer_count += 1;
    printf("added to the buffer. now contains %d\n", thread_control.buffer_count);
  }
  else
  {
    //buffer is full, overwrite the oldest entry and move the head to the next oldest one
    buffer_tail = thread_control.buffer_head;
    thread_control.buffer_head = (thread_control.buffer_head + 1) % BUFFER_SIZE;
    printf("overwriting %s in the buffer\n", thread_control.buffer[buffer_tail]);
    strncpy(thread_control.buffer[buffer_tail], filename, sizeof thread_control.buffer[buffer_tail]);
  }
  pthread_mutex_unlock(&(thread_control.buffer_lock));

  ev_async_send(thread_control.EV_A, &(thread_control.process_data));
}

// When writing these settings we must lock
inline void dwt_update_settings(void* data)
{
    // tell the DUMMY_WORKER thread to copy the settings from the pointers we gave it
    pthread_mutex_lock(&(thread_control.settings_lock));
    memcpy(&dummy_settings, data, sizeof(dummy_settings));
    pthread_mutex_unlock(&(thread_control.settings_lock));
    ev_async_send(thread_control.EV_A, &(thread_control.update_settings));
}


static void server_on_connection(EV_P_ struct evn_server* server, struct evn_stream* stream)
{
  puts("[Server CB] accepted a stream");
  stream->on_data = stream_on_data;
  stream->on_close = stream_on_close;
  stream->server = server;
  stream->oneshot = true;
}

static void stream_on_close(EV_P_ struct evn_stream* stream, bool had_error)
{
  puts("[Stream CB] Close");
}

// This callback is called when stream data is available
static void stream_on_data(EV_P_ struct evn_stream* stream, void* raw, int n)
{
  char filename[32];
  char code = ((char*)raw)[0];
  void* data = (void*) &((char*)raw)[1];
  n -= 1;

  stream->type = code;

  printf("[Data CB] - stream has become readable (%d bytes)\n", n);

  switch (stream->type) {
  case 's':
    printf("\t's' - update dummy_settings");

    if (sizeof(dummy_settings) != n) {
      printf("expected=%d actual=%d", (int) sizeof(dummy_settings), n);
      return;
    }

    dwt_update_settings(data);

    break;


  case '.':
    printf("\t'.' - continue working");


    // TODO dummy data
    if (sizeof(filename) != n) {
      printf("expected=%d actual=%d", (int) sizeof(filename), n);
      return;
    }

    memset(filename, 0, sizeof(filename));
    memcpy(filename, data, sizeof(filename));

    dwt_enqueue(filename);
    break;


  case 'x':
    printf("\t'x' - received kill message; exiting");

    if (0 != n) {
      printf("expected=%d actual=%d", (int) 0, n);
      return;
    }

    ev_unloop(EV_A_ EVUNLOOP_ALL);
    break;


  default:
    printf("\t'%c' (%d) - received unknown message; exiting", stream->type, stream->type);

    ev_unloop(EV_A_ EVUNLOOP_ALL);
  }

  free(raw);
  puts(".");
}
