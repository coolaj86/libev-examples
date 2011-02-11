#ifndef _EVN_H_
#define _EVN_H_

#include <sys/time.h> // struct timeval

// Network Stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>

#include <ev.h>

#include "evn-buffer-list.h"
#include "evn-inbuf.h"
#include "evn-bool.h"

#ifndef EVN_DEBUG
  #if defined DEBUG
    #define EVN_DEBUG 1
  #else
    #define EVN_DEBUG 0
  #endif
#endif

#if EVN_DEBUG
  #include <stdio.h>
  #define evn_debug(...) printf("[EVN] " __VA_ARGS__)
  #define evn_debugs(...) puts("[EVN] " __VA_ARGS__)
#else
  #define evn_debug(...)
  #define evn_debugs(...)
#endif


struct evn_stream;
struct evn_server;
struct evn_exception;

#define EVN_SERVER_P struct evn_server* server
#define EVN_STREAM_P struct evn_stream* stream

// Server Callbacks
typedef void (evn_server_on_listen)(EV_P_ struct evn_server* server);
typedef void (evn_server_on_connection)(EV_P_ struct evn_server* server, struct evn_stream* stream);
typedef void (evn_server_on_close)(EV_P_ struct evn_server* server);

// Client Callbacks
typedef void (evn_stream_on_connect)(EV_P_ struct evn_stream* stream);
typedef void (evn_stream_on_secure)(EV_P_ struct evn_stream* stream); // TODO Implement
typedef void (evn_stream_on_data)(EV_P_ struct evn_stream* stream, void* blob, int size);
typedef void (evn_stream_on_end)(EV_P_ struct evn_stream* stream);
typedef void (evn_stream_on_timeout)(EV_P_ struct evn_stream* stream);
typedef void (evn_stream_on_drain)(EV_P_ struct evn_stream* stream);
typedef void (evn_stream_on_error)(EV_P_ struct evn_stream* stream, struct evn_exception* error);
typedef void (evn_stream_on_close)(EV_P_ struct evn_stream* stream, bool had_error);

typedef enum
{
  evn_CLOSED,
  evn_OPEN,
  evn_OPENING,
  evn_READ_ONLY,
  evn_WRITE_ONLY
} evn_ready_state;

struct evn_exception {
  int error_number;
  char message[256];
};

struct evn_server {
  ev_io io;
  EV_P;
  evn_server_on_listen* on_listen;
  evn_server_on_connection* on_connection;
  evn_server_on_close* on_close;
  struct sockaddr* socket;
  int socket_len;
  //array streams;
};

struct evn_stream {
  ev_io io;
  evn_stream_on_connect* on_connect;
  evn_stream_on_secure* on_secure;
  evn_stream_on_data* on_data;
  evn_stream_on_end* on_end;
  evn_stream_on_timeout* on_timeout;
  evn_stream_on_drain* on_drain;
  evn_stream_on_error* on_error;
  evn_stream_on_close* on_close;
  evn_ready_state ready_state;
  int index;
  bool oneshot;
  evn_bufferlist* bufferlist;
  evn_inbuf* _priv_out_buffer;
  struct evn_server* server;
  struct sockaddr* socket;
  int socket_len;
  EV_P;
  char type;
  void* send_data;
};


int evn_set_nonblock(int fd);

struct evn_server* evn_server_create(EV_P_ evn_server_on_connection* on_connection);
int evn_server_listen(struct evn_server* server, int port, char* address);
void evn_server_priv_on_connection(EV_P_ ev_io *w, int revents);
int evn_server_close(EV_P_ struct evn_server* server);
int evn_server_destroy(EV_P_ struct evn_server* server);

struct evn_stream* evn_stream_create(int fd);
inline struct evn_stream* evn_create_connection(EV_P_ int port, char* address);
struct evn_stream* evn_create_connection_unix_stream(EV_P_ char* sock_path);
struct evn_stream* evn_create_connection_tcp_stream(EV_P_ int port, char* address);
void evn_stream_priv_on_read(EV_P_ ev_io *w, int revents);
bool evn_stream_write(EV_P_ struct evn_stream* stream, void* data, int size);
bool evn_stream_end(EV_P_ struct evn_stream* stream);
bool evn_stream_destroy(EV_P_ struct evn_stream* stream);

#endif
