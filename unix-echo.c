#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <ev.h>

//#include <resolv.h>
#include <unistd.h>

#include "array_heap.h"

#define BUF_SIZE        4096

/*
int serverd; // socket descriptor
struct sockaddr_un server;
int server_len = sizeof(server);
*/
struct sockaddr_un client;
int client_len = sizeof(client);
char buffer[BUF_SIZE];

// This struct should work generically with any socket
struct socket_io;
struct socket_io {
  ev_io io;
  int fd;
  struct sockaddr_un socket;
  int socket_len;
  // if this is a server, it has clients
  array clients;
  // if this is a client, it belongs to a server
  struct socket_io* server;
};

// This callback is called when data is readable on the UDP socket.
static void socket_cb(EV_P_ ev_io *w, int revents) {
    int client_fd;
    puts("unix stream socket has become readable");

    // since ev_io is the first member,
    // watcher `w` has the address of the 
    // start of the socket_io struct
    struct socket_io* server = (struct socket_io*) w;

    client_fd = accept(server->fd, (struct sockaddr *)&client, (socklen_t*)&client_len);
    if (-1 == client_fd) {
      perror("accepting client");
      exit(EXIT_FAILURE);
    }

    int done;
    int n;
    char str[100];

    done = 0;
    do {
      n = recv(client_fd, str, 100, 0);
      if (n <= 0) {
        if (n < 0) {
          perror("recv");
        }
        done = 1;
      }

      if (!done) {
        printf("socket client said: %s", str);
        // not finished yet
        /*
        if (send(client_fd, str, n, 0) < 0) {
          perror("send");
          done = 1;
        }
        */
      }
    } while (!done);

    close(client_fd);

    //socklen_t bytes = recvfrom(serverd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*) &server, (socklen_t *) &server_len);

    // add a null to terminate the input, as we're going to use it as a string
    //buffer[bytes] = '\0';

    //printf("socket client said: %s", buffer);

    // Echo it back.
    // WARNING: this is probably not the right way to do it with libev.
    // Question: should we be setting a callback on sd becomming writable here instead?
    //sendto(sd, buffer, bytes, 0, (struct sockaddr*) &addr, sizeof(addr));
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

int server_init(struct socket_io* server, char* sock_path, int max_queue) {
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
    struct socket_io server;
    struct ev_loop* loop = NULL;

    // Create unix socket in non-blocking fashion
    server_init(&server, "/tmp/libev-echo.sock", max_queue);

    // Create our single-loop for this signle-thread application
    loop = ev_default_loop(0);

    // Get notified whenever the socket is ready to read
    ev_io_init(&server.io, socket_cb, server.fd, EV_READ);
    ev_io_start(loop, &server.io);

    // Run our loop
    puts("unix-socket-echo server starting...");
    ev_loop(loop, 0);

    // This point is only ever reached if the loop is manually exited
    close(server.fd);
    return EXIT_SUCCESS;
}
