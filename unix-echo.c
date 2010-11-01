#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <ev.h>
//#include <glib.h>
#define g_warning printf

//#include <resolv.h>
#include <unistd.h>

#include "array_heap.h"

#define BUF_SIZE        4096

int setnonblock(int fd);
static void not_blocked(EV_P_ ev_periodic *w, int revents);

/*
int serverd; // socket descriptor
struct sockaddr_un server;
int server_len = sizeof(server);
*/
struct sockaddr_un client;
int client_len = sizeof(client);
char buffer[BUF_SIZE];

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

// This callback is called when client data is available
static void client_cb(EV_P_ ev_io *w, int revents) {
  puts("a client has become readable");

  struct sock_ev_client* client = (struct sock_ev_client*) w;

  int n;
  char str[100] = ".\0";

  while (1) {
    n = recv(client->fd, str, 100, 0);
    if (n <= 0) {
      if (0 == n) {
        // an orderly disconnect
        puts("orderly disconnect");
        ev_io_stop(EV_A_ &client->io);
        close(client->fd);
      } else if (EAGAIN == errno) {
        break;
      } else if (EBADF == errno) {
        puts("EBADF: appears that the client is MIA, closing the connection");
        ev_io_stop(EV_A_ &client->io);
        close(client->fd);
      } else if (EBADFD == errno) {
        puts("EBADFD: appears that the client is MIA, closing the connection");
        ev_io_stop(EV_A_ &client->io);
        close(client->fd);
      } else {
        printf("errno: %i ", errno);
        perror("recv");
      }
      break;
    } 
    printf("socket client said: %s", str);
      // not finished yet
      /*
      if (send(client_fd, str, n, 0) < 0) {
        perror("send");
        done = 1;
      }
      */
  };

    //socklen_t bytes = recvfrom(serverd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*) &server, (socklen_t *) &server_len);

    // add a null to terminate the input, as we're going to use it as a string
    //buffer[bytes] = '\0';

    //printf("socket client said: %s", buffer);

    // Echo it back.
    // WARNING: this is probably not the right way to do it with libev.
    // Question: should we be setting a callback on sd becomming writable here instead?
    //sendto(sd, buffer, bytes, 0, (struct sockaddr*) &addr, sizeof(addr));
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
    client_fd = accept(server->fd, (struct sockaddr *)&client, (socklen_t*)&client_len);
    if( client_fd == -1 )
    {
      if( errno != EAGAIN && errno != EWOULDBLOCK )
      {
        g_warning("accept() failed errno=%i (%s)",  errno, strerror(errno));
        exit(EXIT_FAILURE);
      }
      break;
    }
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
    EV_P = NULL;
    //struct ev_loop* loop = NULL;

    // Create unix socket in non-blocking fashion
    server_init(&server, "/tmp/libev-echo.sock", max_queue);

    // Create our single-loop for this signle-thread application
    loop = ev_default_loop(0);

    // To be sure that we aren't actually blocking
    ev_periodic_init(&every_few_seconds, not_blocked, 0, 5, 0);
    ev_periodic_start(EV_A_ &every_few_seconds);

    // Get notified whenever the socket is ready to read
    ev_io_init(&server.io, server_cb, server.fd, EV_READ);
    ev_io_start(EV_A_ &server.io);

    // Run our loop, ostensibly forever
    puts("unix-socket-echo server starting...");
    ev_loop(EV_A_ 0);

    // This point is only ever reached if the loop is manually exited
    close(server.fd);
    return EXIT_SUCCESS;
}


static void not_blocked(EV_P_ ev_periodic *w, int revents) {
  puts("I'm not blocked");
}
