// Standard Stuff
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// Network Stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>

// Libev
#include <ev.h>

// Common Logging
//#include <glib.h>
#define g_warning printf

// Application Specific
#include "array-heap.h"
#include "my-data.h"

struct my_data data;

struct sock_ev_serv {
  ev_io io;
  int fd;
  struct sockaddr_un socket;
  int socket_len;
  array clients;
};

struct sock_ev_client {
  ev_io io;
  int fd;
  int index;
  struct sock_ev_serv* server;
};

int setnonblock(int fd);
static void not_blocked(EV_P_ ev_periodic *w, int revents);
static void server_cb(EV_P_ ev_io *w, int revents);
inline static struct sock_ev_client* client_new(int fd);
static void client_cb(EV_P_ ev_io *w, int revents);
int server_init(struct sock_ev_serv* server, char* sock_path, int max_queue);
int unix_socket_init(struct sockaddr_un* socket_un, char* sock_path, int max_queue);


// This callback is called when client data is available
static void client_cb(EV_P_ ev_io *w, int revents) {
  // a client has become readable

  struct sock_ev_client* client = (struct sock_ev_client*) w;

  int n;

  printf("[r]");
  n = recv(client->fd, &data, sizeof data, 0);
  if (n <= 0) {
    if (0 == n) {
      // an orderly disconnect
      puts("orderly disconnect");
      ev_io_stop(EV_A_ &client->io);
      close(client->fd);
    }  else if (EAGAIN == errno) {
      puts("should never get in this state with libev");
    } else {
      perror("recv");
    }
    return;
  } 

  // Ping back to let the client know the message was received with success
  if (send(client->fd, ".", strlen("."), 0) < 0) {
    perror("send");
  }
}

inline static struct sock_ev_client* client_new(int fd) {
  struct sock_ev_client* client;

  client = realloc(NULL, sizeof(struct sock_ev_client));
  client->fd = fd;
  //client->server = server;
  setnonblock(client->fd);
  ev_io_init(&client->io, client_cb, client->fd, EV_READ);

  return client;
}

// This callback is called when data is readable on the unix socket.
static void server_cb(EV_P_ ev_io *w, int revents) {
  puts("unix stream socket has become readable");

  int client_fd;
  struct sock_ev_client* client;

  // since ev_io is the first member,
  // watcher `w` has the address of the 
  // start of the sock_ev_serv struct
  struct sock_ev_serv* server = (struct sock_ev_serv*) w;

  while (1)
  {
    client_fd = accept(server->fd, NULL, NULL);
    if( client_fd == -1 )
    {
      if( errno != EAGAIN && errno != EWOULDBLOCK )
      {
        g_warning("accept() failed errno=%i (%s)",  errno, strerror(errno));
        exit(EXIT_FAILURE);
      }
      break;
    }
    puts("accepted a client");
    client = client_new(client_fd);
    client->server = server;
    client->index = array_push(&server->clients, client);
    ev_io_start(EV_A_ &client->io);
  }
}

// Simply adds O_NONBLOCK to the file descriptor of choice
int setnonblock(int fd)
{
  int flags;

  flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

int unix_socket_init(struct sockaddr_un* socket_un, char* sock_path, int max_queue) {
  int fd;

  unlink(sock_path);

  // Setup a unix socket listener.
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == fd) {
    perror("echo server socket");
    exit(EXIT_FAILURE);
  }

  // Set it non-blocking
  if (-1 == setnonblock(fd)) {
    perror("echo server socket nonblock");
    exit(EXIT_FAILURE);
  }

  // Set it as unix socket
  socket_un->sun_family = AF_UNIX;
  strcpy(socket_un->sun_path, sock_path);

  return fd;
}

int server_init(struct sock_ev_serv* server, char* sock_path, int max_queue) {
    server->fd = unix_socket_init(&server->socket, sock_path, max_queue);
    server->socket_len = sizeof(server->socket.sun_family) + strlen(server->socket.sun_path);

    array_init(&server->clients, 128);

    if (-1 == bind(server->fd, (struct sockaddr*) &server->socket, server->socket_len))
    {
      perror("echo server bind");
      exit(EXIT_FAILURE);
    }

    if (-1 == listen(server->fd, max_queue)) {
      perror("listen");
      exit(EXIT_FAILURE);
    }
    return 0;
}

int main(void) {
    int max_queue = 128;
    struct sock_ev_serv server;
    struct ev_periodic every_few_seconds;
    // Create our single-loop for this single-thread application
    EV_P  = ev_default_loop(0);

    // Create unix socket in non-blocking fashion
    server_init(&server, "/tmp/libev-ipc-daemon.sock", max_queue);

    // To be sure that we aren't actually blocking
    ev_periodic_init(&every_few_seconds, not_blocked, 0, 5, 0);
    ev_periodic_start(EV_A_ &every_few_seconds);

    // Get notified whenever the socket is ready to read
    ev_io_init(&server.io, server_cb, server.fd, EV_READ);
    ev_io_start(EV_A_ &server.io);

    // Run our loop, ostensibly forever
    puts("unix-socket-echo starting...\n");
    ev_loop(EV_A_ 0);

    // This point is only ever reached if the loop is manually exited
    close(server.fd);
    return EXIT_SUCCESS;
}


static void not_blocked(EV_P_ ev_periodic *w, int revents) {
  char mode[20];

  switch(data.mode)
  {
    case NRC_TURBO:
      strcpy(mode, "turbo");
      break;
    case NRC_FAST:
      strcpy(mode, "fast");
      break;
    case NRC_SLOW:
      strcpy(mode, "slow");
      break;
    case NRC_REALTIME:
      strcpy(mode, "realtime");
      break;
    default:
      strcpy(mode, "ERROR STATE");
  }

  puts("Current Settings:");
  printf("file: %s\n", data.file);
  printf("mode: %s\n", mode);
}
