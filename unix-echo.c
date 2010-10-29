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

#define SOCK_PATH "/tmp/libev_echo_socket"

#define BUF_SIZE        4096

int serverd; // socket descriptor
int clientd; // socket descriptor
struct sockaddr_un server;
struct sockaddr_un client;
int server_len = sizeof(server);
int client_len = sizeof(client);
char buffer[BUF_SIZE];

// This callback is called when data is readable on the UDP socket.
static void socket_cb(EV_P_ ev_io *w, int revents) {
    puts("unix stream socket has become readable");
    socklen_t bytes = recvfrom(serverd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*) &server, (socklen_t *) &server_len);

    // add a null to terminate the input, as we're going to use it as a string
    buffer[bytes] = '\0';

    printf("socket client said: %s", buffer);

    // Echo it back.
    // WARNING: this is probably not the right way to do it with libev.
    // Question: should we be setting a callback on sd becomming writable here instead?
    //sendto(sd, buffer, bytes, 0, (struct sockaddr*) &addr, sizeof(addr));
}

int setnonblock(int fd)
{
  int flags;

  flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

int main(void) {
    puts("unix-socket-echo server started...");

    // Setup a unix socket listener.
    serverd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == serverd) {
      perror("echo server socket");
      exit(EXIT_FAILURE);
    }

    if (-1 == setnonblock(serverd)) {
      perror("echo server socket nonblock");
      exit(EXIT_FAILURE);
    }

    bzero(&server, server_len);
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCK_PATH);
    if (bind(serverd, (struct sockaddr*) &server, server_len) != 0)
    {
      perror("echo server bind");
      exit(EXIT_FAILURE);
    }

    
    // Do the libev stuff.
    struct ev_loop *loop = ev_default_loop(0);
    ev_io socket_watcher;
    ev_io_init(&socket_watcher, socket_cb, serverd, EV_READ);
    ev_io_start(loop, &socket_watcher);
    ev_loop(loop, 0);

    // This point is never reached.
    close(serverd);
    return EXIT_SUCCESS;
}
