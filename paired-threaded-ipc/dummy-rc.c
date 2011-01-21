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

// Nasty globals for now
// feel free to fork this example and clean it up
EV_P;
ev_io daemon_w;
ev_io send_w;
int daemon_fd;

static void evn_client_priv_read_cb (EV_P_ ev_io *w, int revents)
{
  int s_sent1, s_sent2;

  if (revents & EV_WRITE)
  {
    puts ("[dummy-rc] - now ready for writing...");
    if (true == kill_process)
    {
      s_sent1 = send(daemon_fd, "x", sizeof(char), 0);
      if (-1 == s_sent1)
      {
        perror("echo send");
        exit(EXIT_FAILURE);
      }
      puts ("[dummy-rc] - send kill to daemon");
    }
    else if (true == trigger_process)
    {
      s_sent1 = send(daemon_fd, ".", sizeof(char), 0);
      s_sent2 = send(daemon_fd, filename, sizeof(filename), 0);
      if ( (-1 == s_sent1) || (-1 == s_sent2) )
      {
        perror("echo send");
        exit(EXIT_FAILURE);
      }
      else if (sizeof(filename) != s_sent2)
      {
        printf("what the heck? mismatch size? crazy!\n");
      }
    }
    else
    {
      s_sent1 = send(daemon_fd, "s", sizeof(char), 0);
      s_sent2 = send(daemon_fd, &dummy_settings, sizeof dummy_settings, 0);
      if ( (-1 == s_sent1) || (-1 == s_sent2) )
      {
        perror("echo send");
        exit(EXIT_FAILURE);
      }
      else if (sizeof dummy_settings != s_sent2)
      {
        printf("what the heck? mismatch size? crazy!\n");
      }
    }

    // once the data is sent, stop notifications that
    // data can be sent until there is actually more
    // data to send
    close(daemon_fd);
    ev_io_stop(EV_A_ &send_w);
    puts ("[dummy-rc] - stopped listening - all events completed already");
    ev_unloop(EV_A_ EVUNLOOP_ALL);
    //ev_io_set(&send_w, daemon_fd, EV_READ);
    //ev_io_start(EV_A_ &send_w);
  
  }
  else if (revents & EV_READ)
  {
    puts ("[dummy-rc] - now ready for reading...");
    // TODO ACK / NACK
  }
}

static void daemon_cb (EV_P_ ev_io *w, int revents)
{
  puts ("[dummy-rc] - new connection - now ready to send to dummyd");

  // Once the connection is established, listen for writability
  ev_io_start(EV_A_ &send_w);
  // Once we're connected, that's the end of that
  ev_io_stop(EV_A_ &daemon_w);
}

static int connection_new(EV_P_ char* sock_path) {
  int len;
  struct sockaddr_un daemon;

  if (-1 == (daemon_fd = socket(AF_UNIX, SOCK_STREAM, 0))) {
      perror("socket");
      exit(1);
  }

  // Set it non-blocking
  if (-1 == evn_set_nonblock(daemon_fd)) {
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
  ev_io_init(&send_w, evn_client_priv_read_cb, daemon_fd, EV_WRITE);

  daemon.sun_family = AF_UNIX;
  strcpy(daemon.sun_path, sock_path);
  len = strlen(daemon.sun_path) + sizeof(daemon.sun_family);

  if (-1 == connect(daemon_fd, (struct sockaddr *)&daemon, len)) {
      perror("connect");
      exit(EXIT_FAILURE);
  }
  
  return 0;
}

int main (int argc, char* argv[])
{
  //set ALL the default values
  dummy_settings_set_presets(&dummy_settings);
  
  // Parse our arguments; every option seen by parse_opt will be reflected in the globals
  argp_parse (&argp, argc, argv, 0, 0, &dummy_settings);

  // libev handling
  EV_A = EV_DEFAULT;
  
    //connection_new(EV_A_ "/tmp/libev-ipc-daemon.sock");
    char socket_address[256];
    snprintf(socket_address, sizeof socket_address, "/tmp/libev-ipc-daemon.%d.sock", (int)getuid());
    connection_new(EV_A_ socket_address);

  // now wait for events to arrive
  ev_loop(EV_A_ 0);

  // doesn't get here unless unloop is called
  return 0;
}
