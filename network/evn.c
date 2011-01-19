#include "evn.h"
#include <stdlib.h> // realloc, calloc

// Network Stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h> // errno
#include <stdio.h> // perror
#include <arpa/inet.h> // inet_addr

// Prototypes
inline static int evn_set_nonblock(int fd);

inline static struct evn_stream* evn_stream_create(int fd);
inline static struct evn_server* evn_server_new();
inline static struct sockaddr_un* evn_sockaddr_un_new();
inline static int evn_server_unix_create(int* fd, struct sockaddr_un* socket_un, char* sock_path);
inline static struct sockaddr_in* evn_sockaddr_in_new();
inline static int evn_server_ipv4_create(int* fd, struct sockaddr_in* socket_in, int port, char* sock_path);

static int evn_server_bind(EVN_SRV_P);

// Private Callbacks
static void evn_server_listen_priv_cb(EV_P_ ev_io* w, int revents);
static void evn_stream_read_priv_cb(EV_P_ ev_io* w, int revents);
static void evn_server_connection_priv_cb(EV_P_ ev_io* w, int revents);

inline static struct evn_stream*
evn_stream_new()
{
  evn_debug("calloc'd new stream\n");
  return (struct evn_stream*) calloc(1, sizeof(struct evn_stream));
}

inline static struct evn_server* 
evn_server_new()
{
  //return (struct evn_server*) realloc(NULL, sizeof(struct evn_server));
  return (struct evn_server*) calloc(1, sizeof(struct evn_server));
}

inline static struct sockaddr_un* 
evn_sockaddr_un_new()
{
  return (struct sockaddr_un*) calloc(1, sizeof(struct sockaddr_un));
}

inline static struct sockaddr_in* 
evn_sockaddr_in_new()
{
  return (struct sockaddr_in*) calloc(1, sizeof(struct sockaddr));
}

static int
evn_set_nonblock(int fd)
{
  int flags;

  flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}


struct evn_server*
evn_server_create(EV_P_ struct evn_server_callbacks callbacks)
{
  EVN_SRV_P = evn_server_new();
  /*
  if (NULL == EVN_SRV_A) {
    // TODO evn_perror(); if DEBUG
    return NULL;
  }
  */
  EVN_SRV_A->EV_A = EV_A;
  EVN_SRV_A->priv.callbacks.listen = callbacks.listen;
  EVN_SRV_A->priv.callbacks.connection = callbacks.connection;
  EVN_SRV_A->priv.callbacks.close = callbacks.close;
  EVN_SRV_A->priv.max_queue = SOMAXCONN;

  return EVN_SRV_A;
}

int
evn_server_listen(EVN_SRV_P, int port, char* address)
{
  evn_debugs("binding...");

  if (0 == port)
  {
    EVN_SRV_A->priv.socket = (struct sockaddr*) evn_sockaddr_un_new();
    EVN_SRV_A->priv.socket_len = evn_server_unix_create(&EVN_SRV_A->fd, (struct sockaddr_un*) EVN_SRV_A->priv.socket, address);
  }
  else
  {
    EVN_SRV_A->priv.socket = (struct sockaddr*) evn_sockaddr_in_new();
    EVN_SRV_A->priv.socket_len = evn_server_ipv4_create(&EVN_SRV_A->fd, (struct sockaddr_in*) &EVN_SRV_A->priv.socket, port, address);
  }
  // TODO if address is ipv6

  // Set it non-blocking
  if (-1 == evn_set_nonblock(EVN_SRV_A->fd)) {
    perror("echo server socket nonblock");
    exit(EXIT_FAILURE);
  }

  ev_io_init(&EVN_SRV_A->io, evn_server_listen_priv_cb, EVN_SRV_A->fd, EV_READ | EV_WRITE);
  ev_io_start(EVN_SRV_A->EV_A_ &EVN_SRV_A->io);

  evn_server_bind(EVN_SRV_A);

  evn_debugs(" bound");

  return 0;
}

static void
evn_server_listen_priv_cb(EV_P_ ev_io* w, int revents)
{
  EVN_SRV_P = (struct evn_server*) w;

  evn_debugs("listen");

  if (NULL != EVN_SRV_A->priv.callbacks.listen)
  {
    EVN_SRV_A->priv.callbacks.listen(EVN_SRV_A);
  }

  ev_io_stop(EVN_SRV_A->EV_A_ &EVN_SRV_A->io);
  ev_io_init(&EVN_SRV_A->io, evn_server_connection_priv_cb, EVN_SRV_A->fd, EV_READ);
  ev_io_start(EVN_SRV_A->EV_A_ &EVN_SRV_A->io);
}

static void
evn_server_connection_priv_cb(EV_P_ ev_io* w, int revents)
{
  EVN_SRV_P = (struct evn_server*) w;
  EVN_STR_P;
  int stream_fd;

  evn_debugs("connection");
  
  while(true)
  {
    stream_fd = accept(EVN_SRV_A->fd, NULL, NULL);
    if (stream_fd == -1)
    {
      if (errno != EAGAIN && errno != EWOULDBLOCK)
      {
        fprintf(stderr, "accept() failed errno=%i (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
      }
      evn_debugs("done accepting");
      return;
    }
    EVN_STR_A = evn_stream_create(EVN_SRV_A->fd);
    EVN_STR_A->EVN_SRV_A = EVN_SRV_A;
    if (NULL != EVN_SRV_A->priv.callbacks.connection)
    {
      EVN_SRV_A->priv.callbacks.connection(EVN_SRV_A_ EVN_STR_A);
    }
    // user callbacks assigned in step above, then start
    ev_io_start(EVN_SRV_A->EV_A_ &EVN_STR_A->io);
    //client->index = array_push(&server->clients, client);
    evn_debugs("added EV_READ callback to new stream");
  }
}

int
evn_server_bind(EVN_SRV_P)
{
  // TODO array_init(&SVN_SRV_A->clients, 128);
  
  /*
  int optval = 1;

  if (-1 == setsockopt(SVN_SRV_A->fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))) {
    perror("setting SO_REUSEPORT");
    exit(EXIT_FAILURE);
  }

  if (-1 == setsockopt(SVN_SRV_A->fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
    perror("setting SO_REUSEADDR");
    exit(EXIT_FAILURE);
  }
  */

  if (-1 == bind(EVN_SRV_A->fd, EVN_SRV_A->priv.socket, EVN_SRV_A->priv.socket_len))
  {
    perror("echo server bind");
    exit(EXIT_FAILURE);
  }

  if (-1 == listen(EVN_SRV_A->fd, EVN_SRV_A->priv.max_queue)) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int
evn_server_set_max_queue(EVN_SRV_P, int max_queue)
{
  if (EVN_SRV_A->priv.connection) {
    return -1;
  }
  EVN_SRV_A->priv.max_queue = max_queue;
  return 0;
}

int
evn_server_close(EVN_SRV_P)
{
  // close all streams and call close callback
  ev_io_stop(EVN_SRV_A->EV_A_ &EVN_SRV_A->io);
  // TODO join all listeners
  return -1;
}

inline static int
evn_server_unix_create(int* fd, struct sockaddr_un* socket_un, char* sock_path)
{

  // Setup a unix socket listener.
  *fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == *fd) {
    perror("echo server socket");
    exit(EXIT_FAILURE);
  }

  // Set it as unix socket
  socket_un->sun_family = AF_UNIX;
  strcpy(socket_un->sun_path, sock_path);

  return sizeof(socket_un->sun_family) + strlen(socket_un->sun_path);
}

inline static int
evn_server_ipv4_create(int* fd, struct sockaddr_in* socket_in, int port, char* ipv4_addr)
{
  // Setup an ipv4 socket listener.
  *fd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == *fd) {
    perror("echo server socket");
    exit(EXIT_FAILURE);
  }

  // Set it as an ipv4 socket
  socket_in->sin_family = AF_INET;
  socket_in->sin_addr.s_addr = inet_addr(ipv4_addr);
  socket_in->sin_port = htons(port);

  return sizeof(struct sockaddr);
}

inline static struct evn_stream*
evn_stream_create(int stream_fd)
{
  EVN_STR_P;

  evn_debugs("creating stream...");

  EVN_STR_A = evn_stream_new();
  EVN_STR_A->fd = stream_fd;
  evn_set_nonblock(EVN_STR_A->fd);
  ev_io_init(&EVN_STR_A->io, evn_stream_read_priv_cb, EVN_STR_A->fd, EV_READ);

  evn_debugs(" ...stream created");

  return EVN_STR_A;
}

static void
evn_stream_read_priv_cb(EV_P_ ev_io* w, int revents)
{
  int length;
  //struct evn_data* data;
  EVN_STR_P = (struct evn_stream*) w;

  evn_debugs("read begun");

  char xyz[256];

  // Bad File Descriptor
  //length = recv(123, NULL, 0, MSG_TRUNC | MSG_DONTWAIT);
  length = recv(EVN_STR_A->fd, (void *) xyz, (size_t) 15, 0);
  //length = recv(EVN_STR_A->fd, NULL, 0, MSG_TRUNC | MSG_DONTWAIT);
  if (length < 0)
  {
    perror("recv'ing read buffer");
  }
  evn_debug("length: %i\n" ,length);
  if (0 == length) {
    evn_debugs("stream received FIN");
    if (NULL != EVN_STR_A->callbacks.end)
    {
      EVN_STR_A->callbacks.end(EVN_STR_A->EVN_SRV_A_ EVN_STR_A);
    }
    /*
    if (NULL != EVN_SRV_A->callbacks.close)
    {
      EVN_STR_A->callbacks.close(EVN_STR_A);
    }
    */
    return;
  }
  
  if (length > 0 && NULL != EVN_STR_A->callbacks.data)
  {
    evn_debugs("stream received DATA");
    /*
    data = evn_data_new(length);
    if (length != recv(EVN_STR_A->fd, data, length, 0))
    {
      perror("couldn't get data which was previously available");
    }
    EVN_STR_A->callbacks.data(EVN_STR_A, data);
    evn_data_destroy(data);
    */
    EVN_STR_A->callbacks.data(EVN_STR_A->EVN_SRV_A_ EVN_STR_A_ length);
    return;
  }
}

inline void
evn_stream_close(EVN_STR_P)
{
  int result;

  evn_debugs("user closed stream");

  ev_io_stop(EVN_STR_A->EVN_SRV_A->EV_A_ &EVN_STR_A->io);
  result = close(EVN_STR_A->fd);
  if (NULL != EVN_STR_A->callbacks.close)
  {
    // TODO be able to send the correct value
    EVN_STR_A->callbacks.close(EVN_STR_A->EVN_SRV_A_ EVN_STR_A_ false);
  }
  // TODO free alloc`d members
  free(EVN_STR_A);
  return;
}

inline int 
evn_stream_set_timeout(EVN_STR_P_ struct timeval tp)
{
  puts("set_timeout not implemented yet");
  return -1;
}
