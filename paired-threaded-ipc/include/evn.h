#ifndef _EVN_H_
#define _EVN_H_

#include <sys/time.h> // struct timeval

// Network Stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>

#include <ev.h>

struct evn_server {
  ev_io io;
  int fd;
  struct sockaddr_un socket;
  int socket_len;
  //array streams;
};

struct evn_stream {
  ev_io io;
  int fd;
  int index;
  struct evn_server* server;
  char type;
};


int evn_set_nonblock(int fd);
inline static struct evn_stream* evn_stream_create(int fd);
int evn_server_unix_create(struct sockaddr_un* socket_un, char* sock_path);
int evn_server_create(struct evn_server* server, char* sock_path, int max_queue);
int evn_stream_destroy(EV_P_ struct evn_stream* stream);

void evn_server_connection_priv_cb(EV_P_ ev_io *w, int revents);
void evn_stream_read_priv_cb(EV_P_ ev_io *w, int revents);

#endif
