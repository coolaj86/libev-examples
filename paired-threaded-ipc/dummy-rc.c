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

//
// Client Callbacks
//

static void on_close(EV_P_ struct evn_stream* stream, bool had_error)
{
  puts("[dummy-rc]\n\tClose CB");
}

static void on_drain(EV_P_ struct evn_stream* stream)
{
  bool all_sent;
  puts("[dummy-rc]\n\tnow ready for writing...");

  if (true == kill_process)
  {
    all_sent = evn_stream_write(EV_A_ stream, "x", sizeof(char));
  }
  else if (true == trigger_process)
  {
    evn_stream_write(EV_A_ stream, ".", sizeof(char));
    all_sent = evn_stream_write(EV_A_ stream, filename, sizeof(filename));
  }
  else // send settings
  {
    evn_stream_write(EV_A_ stream, "s", sizeof(char));
    all_sent = evn_stream_write(EV_A_ stream, &dummy_settings, sizeof(dummy_settings));
  }

  // 
  if (false == all_sent)
  {
    puts("[dummy-rc]");
    puts("\tall or part of the data was queued in user memory");
    puts("\t'drain' will be emitted when the buffer is again free");
  }
  else
  {
    puts("[dummy-rc]\n\tSuccess: wrote all data.");
  }

  evn_stream_end(EV_A_ stream);
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
    struct evn_stream* stream = evn_create_connection(EV_A_ 0, socket_address);
    if (stream) {
      stream->on_close = on_close;
      stream->on_drain = on_drain;
    }

  // now wait for events to arrive
  ev_loop(EV_A_ 0);

  // doesn't get here unless unloop is called
  return 0;
}
