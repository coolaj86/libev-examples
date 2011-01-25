#include <stdlib.h>
#include <string.h>
#include "evn.h"

//
// Client Callbacks
//

// Connect
static void on_connect(EV_P_ struct evn_stream* stream)
{
  puts("[Client] Stream On Connect");
}

// Secure
static void on_secure(EV_P_ struct evn_stream* stream)
{
  puts("[Client] Stream On Secure");
}

// Data
static void on_data(EV_P_ struct evn_stream* stream, void* data, int size)
{
  char* msg = (char*) data;
  puts("[Client] Stream On Data");
  printf("\t%s\n", msg);
}

// End
static void on_end(EV_P_ struct evn_stream* stream)
{
  puts("[Client] Stream On End");

  //evn_stream_end(EV_A_ stream);
}

// Timeout
static void on_timeout(EV_P_ struct evn_stream* stream)
{
  puts("[Client] Stream On Timeout");
}

// Drain
static void on_drain(EV_P_ struct evn_stream* stream)
{
  bool all_sent;
  char* msg = "Hello World!";

  puts("[Client] Stream On Drain");

  all_sent = evn_stream_write(EV_A_ stream, msg, strlen(msg) + 1);

  if (false == all_sent)
  {
    puts("\tall or part of the data was queued in user memory");
    puts("\t'drain' will be emitted when the buffer is again free");
  }
  else
  {
    puts("\t wrote all data in one shot.");
    evn_stream_end(EV_A_ stream);
  }
}

// Error
static void on_error(EV_P_ struct evn_stream* stream, struct evn_exception* error)
{
  puts("[Client] Stream On Error");
}

// Close
static void on_close(EV_P_ struct evn_stream* stream, bool had_error)
{
  puts("[Client] Stream On Close");
}


//
// Run Application
//

int main (int argc, char* argv[])
{
  EV_P = EV_DEFAULT;
  char socket_address[256] = "/tmp/libevnet-echo.%d.sock";
  snprintf(socket_address, sizeof socket_address, socket_address, (int)getuid());
  struct evn_stream* stream = evn_create_connection(EV_A_ 0, socket_address);

  if (stream) {
    stream->on_connect = on_connect;
    stream->on_secure = on_secure;
    stream->on_data = on_data;
    stream->on_end = on_end;
    stream->on_timeout = on_timeout;
    stream->on_drain = on_drain;
    stream->on_error = on_error;
    stream->on_close = on_close;
  }

  ev_loop(EV_A_ 0);
  puts("[Client] No events left");
  return 0;
}
