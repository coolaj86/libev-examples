#include "evn.h"
#include <stdlib.h> // realloc, calloc
#include "stdio.h" // puts
#include "stdio.h" // puts
#include <unistd.h> // unlink

struct my_state {
  bool acquiring;
  char type;
  char* data;
  long long size;
};

inline static struct my_state*
my_state_new()
{
  return (struct my_state*) calloc(1, sizeof(struct my_state));
}

/*
 * Client Event Handlers
 *
 */
void str_connect(struct evn_server* server, struct evn_stream* client)
{
  puts("client ready to Send and Receive data");
  struct timeval tp = {0, 500000}; // 100ms
  evn_stream_set_timeout(client, tp); // us
}

void str_data(struct evn_server* server, struct evn_stream* client, int size)
{
  struct my_state* state = (struct my_state*) client->data;


  // TODO create inline callback / macro
  state->data = malloc((size_t) size);
  if (size != recv(client->fd, state->data, size, 0))
  {
    puts("very much unexpected size");
  }
  else
  {
    puts("received expected size");
  }

  if (state->acquiring)
  {
    // add this data to prior data
    puts("aggregating data");
  }
  else 
  {
    switch (state->data[0]) {
    case '.':
      // do something
      puts("received '.'");
      break;
    case 's':
      // update settings
      puts("received 's'");
      break;
    case 'x':
      // close and quit
      puts("received 'x'");
      evn_server_close(server);
      break;
    default:
      // accumulate data
      puts("received unknown signal");
      break;
    }
  }
  free(state->data);
}

void str_end(struct evn_server* server, struct evn_stream* client)
{
  evn_stream_close(client);
  puts("client disconnected");
}

void str_timeout(struct evn_server* server, struct evn_stream* client)
{
  evn_stream_close(client);
  puts("client timed out (and was forced to disconnect)");
}

void str_close(struct evn_server* server, struct evn_stream* client, bool had_error)
{
  puts("disconnect complete");
}


/*
 * Server Event Handlers
 *
 */
void srv_connection(struct evn_server* server, struct evn_stream* client)
{
  puts("A client stream connection is established");
  struct evn_stream_callbacks* cbs = &client->callbacks;

  cbs->connect = str_connect;
  cbs->data = str_data;
  cbs->end = str_end;
  cbs->timeout = str_timeout;
  cbs->close = str_close;

  // store custom data
  client->data = (void*) my_state_new();
}

void srv_close(struct evn_server* server) {
  puts("The server has closed");
}

/*
 * Run Application
 *
 */
int main (int argc, char* argv[])
{
  EV_P;
  EVN_SRV_P;
  struct evn_server_callbacks srv_callbacks = {
    NULL,
    srv_connection,
    srv_close
  };

  EV_A = ev_default_loop(0);

  EVN_SRV_A = evn_server_create(EV_A_ srv_callbacks);
  evn_server_set_max_queue(EVN_SRV_A, 128);
  // We don't care if this did or didn't exist
  unlink("/tmp/evn-test.sock");
  evn_server_listen(EVN_SRV_A, 0, "/tmp/evn-test.sock");
  
  // TODO set timeout
  // evn_server_close(srv);

  ev_loop(EV_A_ 0); 
  return 0;
}
