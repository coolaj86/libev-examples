#ifndef _EVN_H_
#define _EVN_H_

#include <sys/time.h> // struct timeval
#include <sys/socket.h> // socklen_t

#include <ev.h>
#include "bool.h"

// Forward Declaration to circumvent Cirular Reference
struct evn_exception;
struct evn_server;
struct evn_stream;
struct evn_data;

// Function Pointers / callbacks
//typedef void (_USERENTRY* evn_server_listen_user_cb) (struct evn_server*);
typedef void (evn_server_listen_user_cb)(struct evn_server* server);
typedef void (evn_server_connection_user_cb)(struct evn_server* server, struct evn_stream* stream);
typedef void (evn_server_close_user_cb)(struct evn_server* server);

typedef void (evn_stream_connect_user_cb)(struct evn_server* server, struct evn_stream* stream);
typedef void (evn_stream_secure_user_cb)(struct evn_server* server, struct evn_stream* stream);
typedef void (evn_stream_data_user_cb)(struct evn_server* server, struct evn_stream* stream, int size);
typedef void (evn_stream_end_user_cb)(struct evn_server* server, struct evn_stream* stream);
typedef void (evn_stream_timeout_user_cb)(struct evn_server* server, struct evn_stream* stream);
typedef void (evn_stream_drain_user_cb)(struct evn_server* server, struct evn_stream* stream);
typedef void (evn_stream_error_user_cb)(struct evn_server* server, struct evn_stream* stream, struct evn_exception error);
typedef void (evn_stream_close_user_cb)(struct evn_server* server, struct evn_stream* stream, bool had_error);

struct evn_server_callbacks {
  evn_server_listen_user_cb* listen;
  evn_server_connection_user_cb* connection;
  evn_server_close_user_cb* close;
};

struct evn_stream_callbacks {
  evn_stream_connect_user_cb* connect;
  evn_stream_secure_user_cb* secure;
  evn_stream_data_user_cb* data;
  evn_stream_end_user_cb* end;
  evn_stream_timeout_user_cb* timeout;
  evn_stream_drain_user_cb* drain;
  evn_stream_error_user_cb* error;
  evn_stream_close_user_cb* close;
};

struct evn_exception {
  char* message;
  int errno;
};

struct evn_server_priv {
  struct sockaddr* socket;
  socklen_t socket_len;
  int max_queue;
  struct evn_server_callbacks callbacks;
  bool connection;
  // TODO array streams
};

struct evn_server {
  ev_io io;
  int fd;
  EV_P;
  struct evn_server_priv priv;
};

struct evn_stream_priv {
  int index;
  struct timeval timeout;
  struct timeval last_activity;
  ev_tstamp ev_timeout;
  ev_tstamp ev_last_activity;
};

struct evn_stream {
  ev_io io;
  int fd;
  struct evn_server* server;
  struct evn_stream_callbacks callbacks;
  struct evn_stream_priv priv;
  void* data;
};

struct evn_data {
  char* data;
  int size;
};

struct evn_buffer {
  char* data;
  long long size;
  long long index;
};

#define EVN_SRV_P struct evn_server* server
#define EVN_SRV_P_ struct evn_server* server,
#define EVN_SRV_A server
#define EVN_SRV_A_ server,

#define EVN_STR_P struct evn_stream* stream
#define EVN_STR_P_ struct evn_stream* stream,
#define EVN_STR_A stream
#define EVN_STR_A_ stream,

ev_tstamp evn_tstamp_to_ev_tstamp(struct timeval);

struct evn_server* evn_server_create(EV_P_ struct evn_server_callbacks callbacks);
int evn_server_listen(EVN_SRV_P, int port, char* address);
int evn_server_set_max_queue(EVN_SRV_P, int max_queue);
int evn_server_close(EVN_SRV_P);

// evn_create_stream is static and defined in the .c
inline int evn_stream_set_timeout(EVN_STR_P_ struct timeval tp);
void evn_stream_close(EVN_STR_P);
void evn_stream_write(EVN_STR_P);

#endif
