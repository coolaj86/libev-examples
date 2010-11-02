// Arg Parser
#include <argp.h>

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
#include "my-data.h"


/* IPC Data */

// NOTE: unless you have a serialize function, you
// may not use a struct with pointers to send across
// a socket. The reference will be meaningless on
// the other side.
struct my_data data;


/***********************************
 *
 *  Remote Control Argument Parser
 *
 */
const char* argp_program_version = "Newdora Remote v0.1 (Alpha Alligator)";
const char* argp_program_bug_address = "<newdora@example.tld>";

/* Program documentation. */
static char doc[] = "Newdora Remote -- Remote Control (RC) for Newdora Serving Server Services Streamer (N4S)";

/* The options we understand. */
static struct argp_option options[] = {
  {"mode", 'm', "string",   0,  "turbo, fast, slow, or realtime"},
  {"file", 'f', "fullpath", 0,  "the N4S encoded file to render"},
  { 0 }
};

static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct my_data* data = state->input;

  switch (key)
  {
    case 'm':
      if (0 == strcmp("fast", arg))
      {
        data->mode = NRC_FAST;
      }
      else if (0 == strcmp("slow", arg))
      {
        data->mode = NRC_SLOW;
      }
      else if (0 == strcmp("turbo", arg))
      {
        data->mode = NRC_TURBO;
      }
      else if (0 == strcmp("realtime", arg))
      {
        data->mode = NRC_REALTIME;
      }
      else
      {
        return EINVAL;
      }
      break;

    case 'f':
      if (strlen(arg) > 255) {
        return E2BIG;
      }
      strcpy(data->file, arg);
      break;

    case ARGP_KEY_ARG:
      return ARGP_ERR_UNKNOWN;
      break;

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
 *  libev connection client
 *
 */

// Nasty globals for now
// feel free to fork this example and clean it up
EV_P; // Macro for `struct ev_loop* loop`
ev_io daemon_w;
ev_io send_w;
int daemon_fd;

static void send_cb (EV_P_ ev_io *w, int revents)
{
  int s_sent;

  if (revents & EV_WRITE)
  {
    puts ("daemon ready for writing...");

    s_sent = send(daemon_fd, &data, sizeof data, 0);
    if (-1 == s_sent)
    {
      perror("echo send");
      exit(EXIT_FAILURE);
    }
    else if (sizeof data != s_sent)
    {
      printf("what the heck? mismatch size? crazy!\n");
    }
    // once the data is sent, stop notifications that
    // data can be sent until there is actually more 
    // data to send
    ev_io_stop(EV_A_ &send_w);
    ev_unloop(EV_A_ EVUNLOOP_ALL);
    //ev_io_set(&send_w, daemon_fd, EV_READ);
    //ev_io_start(EV_A_ &send_w);
  }
  else if (revents & EV_READ)
  {
    // TODO ACK / NACK
  }
}

static void daemon_cb (EV_P_ ev_io *w, int revents)
{
  puts ("connected, now watching stdin");
  // Once the connection is established, listen for writability
  ev_io_start(EV_A_ &send_w);
  // Once we're connected, that's the end of that
  ev_io_stop(EV_A_ &daemon_w);
}


// Simply adds O_NONBLOCK to the file descriptor of choice
int setnonblock(int fd)
{
  int flags;

  flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

static void connection_new(EV_P_ char* sock_path) {
  int len;
  struct sockaddr_un daemon;

  if (-1 == (daemon_fd = socket(AF_UNIX, SOCK_STREAM, 0))) {
      perror("socket");
      exit(1);
  }

  // Set it non-blocking
  if (-1 == setnonblock(daemon_fd)) {
    perror("echo client socket nonblock");
    exit(EXIT_FAILURE);
  }

  // this should be initialized before the connect() so
  // that no packets are dropped when initially sent?
  // http://cr.yp.to/docs/connect.html

  // initialize the connect callback so that it starts the stdin asap
  ev_io_init (&daemon_w, daemon_cb, daemon_fd, EV_WRITE);
  ev_io_start(EV_A_ &daemon_w);
  // initialize the send callback, but wait to start until there is data to write
  ev_io_init(&send_w, send_cb, daemon_fd, EV_WRITE);
  ev_io_start(EV_A_ &send_w);

  daemon.sun_family = AF_UNIX;
  strcpy(daemon.sun_path, sock_path);
  len = strlen(daemon.sun_path) + sizeof(daemon.sun_family);

  if (-1 == connect(daemon_fd, (struct sockaddr *)&daemon, len)) {
      perror("connect");
      //exit(1);
  }
}

int main (int argc, char* argv[])
{
  /* Argument Parsing */

  // define reasonable defaults
  data.mode = NRC_REALTIME;
  strcpy(data.file, "/usr/local/media/demo.n4s");

  // override defaults with user settings
  argp_parse (&argp, argc, argv, 0, 0, &data);



  /* libev handling */

  loop = EV_DEFAULT;
  connection_new(EV_A_ "/tmp/libev-ipc-daemon.sock");

  // now wait for events to arrive
  ev_loop(EV_A_ 0);

  // doesn't get here unless unloop is called
  return 0;
}

