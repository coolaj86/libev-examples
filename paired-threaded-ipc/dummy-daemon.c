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

//Function Prototypes
void clean_shutdown(int sig);
static void add_to_buffer(char* filename);
static void test_process_is_not_blocked(EV_P_ ev_periodic *w, int revents);
static void stream_on_data(EV_P_ struct evn_stream* stream, void* data, int n);
static void stream_on_close(EV_P_ struct evn_stream* stream, bool had_error);
static void server_on_connection(EV_P_ struct evn_server* server, struct evn_stream* stream);

const char* argp_program_version = "ACME DummyServe v1.0 (Alphabet Animal)";
const char* argp_program_bug_address = "<bugs@example.com>";

/* Program documentation. */
static char doc[] = "ACME DummyServe -- An abstraction layer between the Dummy Processing Algorithms and Business Logic";

//settings structs
static DUMMY_SETTINGS   dummy_settings;
static bool redirect = false;

static pthread_t dsp_thread;
static struct DPROC_THREAD_CONTROL thread_control;

// Create our single-loop for this single-thread application
EV_P;

/* The options we understand. */
static struct argp_option options[] = {
#include "dummy_settings_argp_option.c"
  { 0 }
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
  {
#include "dummy_settings_parse_opt.c"

    case ARGP_KEY_ARG:
      return ARGP_ERR_UNKNOWN;

    case ARGP_KEY_END:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

void clean_shutdown(int sig) {
  void* exit_status;

  fprintf(stderr, "Received signal %d, shutting down\n", sig);
  // TODO handle signal close(server.fd);
  ev_async_send(thread_control.EV_A, &(thread_control.cleanup));

  //this will block until the dsp_thread exits, at which point we will continue with out shutdown.
  pthread_join(dsp_thread, &exit_status);
  ev_loop_destroy (EV_DEFAULT_UC);
  puts("[dummyd] Dummy cleanup finished.");
  exit(EXIT_SUCCESS);
}

// Our argument parser.
static struct argp argp = { options, parse_opt, 0, doc };

int main (int argc, char* argv[])
{
  pthread_attr_t attr;
  int thread_status;
  struct evn_server* server;

  dummy_settings_set_presets(&dummy_settings);

  // Parse our arguments; every option seen by parse_opt will be reflected in settings.
  argp_parse (&argp, argc, argv, 0, 0, &dummy_settings);

  if (true == redirect)
  {
    //these lines are to direct the stdout and stderr to log files we can access even when run as a daemon (after the possible help info is displayed.)
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

  int max_queue = 128;
  EV_A = ev_default_loop(0);

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
  pthread_mutex_init(&(thread_control.settings_lock), NULL);
  thread_control.dummy_settings = &dummy_settings;

  pthread_mutex_init(&(thread_control.buffer_lock), NULL);
  thread_control.buffer_head = 0;
  thread_control.buffer_count = 0;

  thread_control.EV_A = ev_loop_new(EVFLAG_AUTO);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // Initialize the thread that will manage the DPROC
  thread_status = pthread_create(&dsp_thread, &attr, dummy_worker_thread, (void *)(&thread_control));
  if (0 != thread_status)
  {
    fprintf(stderr, "thread creation failed with errno %d (%s)\n", thread_status, strerror(thread_status));
    exit(EXIT_FAILURE);
  }
  pthread_attr_destroy(&attr);


  // Create unix socket in non-blocking fashion
#ifdef TI_DPROC
  server = evn_server_create(EV_A_ "/tmp/libev-ipc-daemon.sock", max_queue);
#else
  char socket_address[256];
  snprintf(socket_address, sizeof socket_address, "/tmp/libev-ipc-daemon.sock%d", (int)getuid());
  server = evn_server_create(EV_A_ socket_address, max_queue);
#endif

  signal(SIGQUIT, clean_shutdown);
  signal(SIGTERM, clean_shutdown);
  signal(SIGINT,  clean_shutdown);

  // TODO assign with create
  server->connection = server_on_connection;

  // Get notified whenever the socket is ready to read
  // TODO assign with listen
  evn_server_listen(server);

  // Run our loop, until we recieve the QUIT, TERM or INT signals, or an 'x' over the socket.
  puts("[dummyd] unix-socket-echo starting...\n");
  ev_loop(EV_A_ 0);

  // This point is only ever reached if the loop is manually exited
  clean_shutdown(0);
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

static void add_to_buffer(char* filename)
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


static void server_on_connection(EV_P_ struct evn_server* server, struct evn_stream* stream)
{
  puts("[Server CB] accepted a stream");
  stream->data = stream_on_data;
  stream->close = stream_on_close;
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

  //char c[1024];
  //n = recv(stream->fd, &c, 1024, MSG_TRUNC | MSG_PEEK);
  printf("[dummyd] - new data - EV_READ - stream has become readable (%d bytes)\n", n);
  if (n < 0)
  {
    perror("recv err");
  }

  /*
  if(0 == stream->type)
  {
    printf("[r]");
    stream->type = data[0];
    //n = recv(stream->fd, &(stream->type), sizeof (stream->type), 0);
    printf("[dummyd] '0' read %d bytes\n", n);
    if (n <= 0) {
      if (0 == n) {
        // an orderly disconnect
        evn_stream_destroy(EV_A_ stream);
      } else if (EAGAIN == errno) {
        fprintf(stderr, "should never get in this state (EAGAIN) with libev\n");
      } else {
        perror("recv type");
      }
      return;
    }
  }
  */

  if ('g' == stream->type) {
    puts("[dummyd] g - dummy_settings");
    usleep(10);

    pthread_mutex_lock(&(thread_control.settings_lock));
    //n = recv(stream->fd, &dummy_settings, sizeof dummy_settings, 0);
    memcpy(&dummy_settings, data, sizeof dummy_settings);
    printf("[dummyd] 'g' read %d bytes\n", n);
    pthread_mutex_unlock(&(thread_control.settings_lock));

    if (sizeof dummy_settings != n) {
      printf("expected=%d actual=%d", (int) sizeof(dummy_settings), n);
      perror("recv dummy struct");
      return;
    }
    //evn_stream_destroy(EV_A_ stream);

    // tell the DPROC thread to copy the settings from the pointers we gave it
    ev_async_send(thread_control.EV_A, &(thread_control.update_settings));
  }
  else if ('.' == stream->type)
  {
    memset(filename, 0, sizeof filename);
    puts("[dummyd] . - process new data (same settings)");
    usleep(10);
    //n = recv(stream->fd, filename, sizeof filename, 0);
    memcpy(filename, data, sizeof filename);
    printf("[dummyd] '.' read %d bytes for the filename %s\n", n, filename);
    if (n <= 0) {
      perror("recv raw data filename");
      return;
    }
    //evn_stream_destroy(EV_A_ stream);
    add_to_buffer(filename);
  }
  else if ('x' == stream->type)
  {
    puts("[dummyd] received 'x' (kill) message - exiting");
    sleep(1);
    //evn_stream_destroy(EV_A_ stream);
    ev_unloop(EV_A_ EVUNLOOP_ALL);
  }
  else
  {
    fprintf(stderr, "unknown socket type. %d, or '%c' not a valid type.\nIgnoring request\n", stream->type, stream->type);
    //evn_stream_destroy(EV_A_ stream);
    exit(EXIT_FAILURE);
  }

  /*
  // Ping back to let the stream know the message was received with success
  if (send(stream->fd, ".", strlen("."), 0) < 0) {
    perror("send");
  }
  */
  puts("[dummyd] done with loop");
}
