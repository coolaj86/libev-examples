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

#define SOCK_PATH "/tmp/libev_echo.sock"

#define BUF_SIZE        4096

/*
int serverd; // socket descriptor
struct sockaddr_un server;
int server_len = sizeof(server);
*/
int clientd; // socket descriptor
struct sockaddr_un client;
int client_len = sizeof(client);
char buffer[BUF_SIZE];

// This struct should work generically with any socket
typedef struct {
  ev_io io;
  int fd;
  struct sockaddr_un socket;
  int socket_len;
} socket_io;

// This callback is called when data is readable on the UDP socket.
static void socket_cb(EV_P_ ev_io *w, int revents) {
    puts("unix stream socket has become readable");

    // since ev_io is the first member,
    // watcher `w` has the address of the 
    // start of the socket_io struct
    socket_io* server = (socket_io*) w;

    clientd = accept(server->fd, (struct sockaddr *)&client, (socklen_t*)&client_len);
    if (-1 == clientd) {
      perror("accepting client");
      exit(EXIT_FAILURE);
    }

    int done;
    int n;
    char str[100];

    done = 0;
    do {
      n = recv(clientd, str, 100, 0);
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
        if (send(clientd, str, n, 0) < 0) {
          perror("send");
          done = 1;
        }
        */
      }
    } while (!done);

    close(clientd);

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

int main(void) {
    int max_queue = 128;
    socket_io server;

    unlink(SOCK_PATH);
    memset(&server, 0, sizeof(socket_io));

    puts("unix-socket-echo server started...");

    // Setup a unix socket listener.
    server.fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == server.fd) {
      perror("echo server socket");
      exit(EXIT_FAILURE);
    }

    if (-1 == setnonblock(server.fd)) {
      perror("echo server socket nonblock");
      exit(EXIT_FAILURE);
    }

    server.socket.sun_family = AF_UNIX;
    strcpy(server.socket.sun_path, SOCK_PATH);
    server.socket_len = sizeof(server.socket.sun_family) + strlen(server.socket.sun_path);

    if (-1 == bind(server.fd, (struct sockaddr*) &server.socket, server.socket_len))
    {
      perror("echo server bind");
      exit(EXIT_FAILURE);
    }

    if (-1 == listen(server.fd, max_queue)) {
      perror("listen");
      exit(EXIT_FAILURE);
    }
    
    // Do the libev stuff.
    struct ev_loop *loop = ev_default_loop(0);
    //ev_io socket_watcher;
    ev_io_init(&server.io, socket_cb, server.fd, EV_READ);
    ev_io_start(loop, &server.io);
    ev_loop(loop, 0);

    // This point is never reached.
    close(server.fd);
    return EXIT_SUCCESS;
}
