#include <stdio.h>
#include <ev.h>

#include "evn.h"

//
// Event Callbacks
//

// Data
static void on_data(EV_P_ struct evn_stream* stream, void* data, int size)
{
  puts("[Server] On Data");
  evn_stream_write(EV_A_ stream, data, size);
  //evn_stream_end(EV_A_ stream);
}

// Close
static void on_stream_close(EV_P_ struct evn_stream* stream, bool had_error)
{
  puts("[Server] On (Stream) Close");
  evn_server_close(EV_A_ stream->server);
}

// Connection
static void on_connection(EV_P_ struct evn_server* server, struct evn_stream* stream)
{
  puts("[Server] On Connection");
  stream->on_data = on_data;
  stream->on_close = on_stream_close;
  stream->oneshot = true;
}

// Listen
static void on_listen(EV_P_ struct evn_server* server)
{
  puts("[Server] On Listen");
}

//
// Run Application
//
int main (int argc, char* argv[])
{
  EV_P = ev_default_loop(0);
  struct evn_server* server;

  // Create unix socket in non-blocking fashion
  char socket_address[256] = "/tmp/libevnet-echo." "%d" ".sock";
  snprintf(socket_address, sizeof socket_address, socket_address, (int)getuid());
  unlink(socket_address);

  server = evn_server_create(EV_A_ on_connection);
  server->on_listen = on_listen;
  evn_server_listen(server, 0, socket_address);

  ev_loop(EV_A_ 0);
  puts("[Server] No events left");
  return 0;
}
