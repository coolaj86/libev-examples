// Arg Parser
#include <argp.h>
#include <string.h>

// Standard Equipment
#include <stdlib.h>
#include <stdio.h>

// libev
#include <ev.h>

// Unix Sockets
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fcntl.h> // fcntl
#include <unistd.h> // close
#include <errno.h>

// Application Specific
#include "dummy-settings.h"
#include "bool.h"

#include "evn.h"

/* IPC Data */

// NOTE: you may not use a struct with pointers to send across
// a socket. The reference will be meaningless on the other side.
static DUMMY_SETTINGS dummy_settings;
static bool kill_process = false;
static bool trigger_process = false;
static char filename[32] = {};


/***********************************
*
* Remote Control Argument Parser
*
*/
const char* argp_program_version = "ACME DummyRemote v1.0 (Alphabet Animal)";
const char* argp_program_bug_address = "<bugs@example.com>";

/* Program documentation. */
static char doc[] = "ACME DummyRemote -- A unix-stream client for controlling DummyServe";

/* The options we understand. */
static struct argp_option options[] = {
  #include "dummy_settings_argp_option.c"
 {"kill"                    , 'x',  0       , 0,  "send the kill command over the socket"},
 {"run"                     , '.',  "file"  , 0,  "send the run command over the socket with the file to process (only the file name not the path. must be in /dev/shm)"},
 { 0 }
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
  {
    case 'x':
      kill_process = true;
      break;
    case '.':
      trigger_process = true;
      strncpy(filename, arg, sizeof filename); //strncpy to make sure we don't exceed the bounds of our array.
      filename[sizeof filename - 1] = '\0';    //just make sure the result is NULL terminated
      break;
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

static struct argp argp = { options, parse_opt, 0, doc };


/***********************************
*
* libev connection client
*
*/
static bool evn_stream_priv_send(struct evn_stream* stream, void* data, int size);

void evn_stream_end(EV_P_ struct evn_stream* stream)
{
  evn_stream_destroy(EV_A_ stream);
}

bool evn_stream_write(EV_P_ struct evn_stream* stream, void* data, int size)
{
  if (NULL == stream->_write_bufferlist)
  {
    if (evn_stream_priv_send(stream, data, size))
    {
      evn_debugs("All data was sent without queueing");
      return true;
    }
    //stream->_write_bufferlist = evn_bufferlist_create(size, 0);
  }
  //evn_bufferlist_add(stream->_write_bufferlist, data, size);

  // Ensure that we listen for EV_WRITE
  if (!(stream->io.events & EV_WRITE))
  {
    ev_io_stop(EV_A_ &stream->io);
    ev_io_set(&stream->io, stream->fd, EV_READ | EV_WRITE);
  }
  ev_io_start(EV_A_ &stream->io);

  return false;
}

static bool evn_stream_priv_send(struct evn_stream* stream, void* data, int size)
{
  const char* test_data = ".awesome_sauce";
  //const int MAX_SEND = 4096;
  int sent;

  evn_debugs("priv_send");
  if (NULL == data)
  {
    evn_debugs("no data, skipping");
    return true;
  }

  // if the buffer exists, append the data to the buffer
  // while sent != 0 and buffer != NULL
  // void* data = bufferlist_peek(stream->bufferlist, MAX_SEND);
  sent = send(stream->fd, test_data, strlen(test_data) + 1, MSG_DONTWAIT);
  // bufferlist_shift_void(stream->bufferlist, sent);

  return sent == strlen(test_data);
}

static void evn_stream_priv_on_write(EV_P_ ev_io *w, int revents)
{
  struct evn_stream* stream = (struct evn_stream*) w;

  evn_debugs("EV_WRITE");

  evn_stream_priv_send(stream, NULL, 0);

  // If the buffer is finally empty, send the `drain` event
  if (NULL == stream->_write_bufferlist)
  {
    ev_io_stop(EV_A_ &stream->io);
    ev_io_set(&stream->io, stream->fd, EV_READ);
    ev_io_start(EV_A_ &stream->io);
    evn_debugs("pre-drain");
    if (stream->drain) { stream->drain(EV_A_ stream); }
    // and the 
    return;
  }
  evn_debugs("post-null");
}

static inline void evn_stream_priv_on_activity(EV_P_ ev_io *w, int revents)
{
  evn_debugs("Stream Activity");
  if (revents & EV_READ)
  {
    // evn_stream_read_priv_cb
  }
  else if (revents & EV_WRITE)
  {
    evn_stream_priv_on_write(EV_A, w, revents);
  }
  else
  {
    // Never Happens
    evn_debugs("[ERR] ev_io received something other than EV_READ or EV_WRITE");
  }
}

static void evn_stream_priv_on_connect(EV_P_ ev_io *w, int revents)
{
  struct evn_stream* stream = (struct evn_stream*) w;
  evn_debugs("Stream Connect");

  //ev_cb_set (ev_TYPE *watcher, callback)
  //ev_io_set (&w, STDIN_FILENO, EV_READ);

  ev_io_stop(EV_A_ &stream->io);
  ev_io_init(&stream->io, evn_stream_priv_on_activity, stream->fd, EV_WRITE);
  ev_io_start(EV_A_ &stream->io);
  //ev_cb_set(&stream->io, evn_stream_priv_on_activity);

  if (stream->connect) { stream->connect(EV_A_ stream); }
}

struct evn_stream* evn_create_connection(EV_P_ char* sock_path) {
  int stream_fd;
  struct evn_stream* stream;
  struct sockaddr_un* sock;

  stream_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == stream_fd) {
      perror("[drc] socket");
      exit(1);
  }
  stream = evn_stream_create(stream_fd);
  sock = &stream->socket;

  // initialize the connect callback so that it starts the stdin asap
  ev_io_init(&stream->io, evn_stream_priv_on_connect, stream_fd, EV_WRITE);
  ev_io_start(EV_A_ &stream->io);

  sock->sun_family = AF_UNIX;
  strcpy(sock->sun_path, sock_path);
  stream->socket_len = strlen(sock->sun_path) + 1 + sizeof(sock->sun_family);

  if (-1 == connect(stream_fd, (struct sockaddr *) sock, stream->socket_len)) {
      perror("connect");
      exit(EXIT_FAILURE);
  }
  
  return stream;
}


//
// Client Callbacks
//

void on_drain(EV_P_ struct evn_stream* stream)
{
  bool overflow;
  puts("[dummy-rc] - now ready for writing...");

  if (true == kill_process)
  {
    overflow = evn_stream_write(EV_A_ stream, "x", sizeof(char));
  }
  else if (true == trigger_process)
  {
    evn_stream_write(EV_A_ stream, ".", sizeof(char));
    overflow = evn_stream_write(EV_A_ stream, filename, sizeof(filename));
  }
  else // send settings
  {
    evn_stream_write(EV_A_ stream, "s", sizeof(char));
    overflow = evn_stream_write(EV_A_ stream, &dummy_settings, sizeof(dummy_settings));
  }

  // 
  if (overflow)
  {
    puts("all or part of the data was queued in user memory");
    puts("'drain' will be emitted when the buffer is again free");
  }

  puts("[dummy-rc] - done writing.");
  evn_stream_end(EV_A_ stream);
  //ev_unloop(EV_A_ EVUNLOOP_ALL);
}

int main (int argc, char* argv[])
{
  //set ALL the default values
  dummy_settings_set_presets(&dummy_settings);
  
  // Parse our arguments; every option seen by parse_opt will be reflected in the globals
  argp_parse (&argp, argc, argv, 0, 0, &dummy_settings);

  // libev handling
  EV_P = EV_DEFAULT;
  
    //evn_create_connection(EV_A_ "/tmp/libev-ipc-daemon.sock");
    char socket_address[256];
    snprintf(socket_address, sizeof socket_address, DUMMYD_SOCK, (int)getuid());
    struct evn_stream* stream = evn_create_connection(EV_A_ socket_address);
    if (stream) {
      stream->drain = on_drain;
    }

  // now wait for events to arrive
  ev_loop(EV_A_ 0);

  // doesn't get here unless unloop is called
  return 0;
}
