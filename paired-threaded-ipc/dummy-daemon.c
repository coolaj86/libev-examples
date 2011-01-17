#include <argp.h>           //the command line argument parser
#include <sys/resource.h>   //setpriority
#include <signal.h>         //signals to process data and quit cleanly

#include "dummy-worker-thread.h"

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
#include <pthread.h>

#include "dummy-settings.h"
#include "bool.h"

#define LOG_PATH "/tmp/"

struct sock_ev_serv {
  ev_io io;
  int fd;
  struct sockaddr_un socket;
  int socket_len;
  //array clients;
};

struct sock_ev_client {
  ev_io io;
  int fd;
  int index;
  struct sock_ev_serv* server;
  char type;
};

//Function Prototypes
int setnonblock(int fd);
static void not_blocked(EV_P_ ev_periodic *w, int revents);
int server_init(struct sock_ev_serv* server, char* sock_path, int max_queue);
int unix_socket_init(struct sockaddr_un* socket_un, char* sock_path, int max_queue);
static void server_cb(EV_P_ ev_io *w, int revents);
inline static struct sock_ev_client* client_new(int fd);
static void client_cb(EV_P_ ev_io *w, int revents);
int close_client(EV_P_ struct sock_ev_client* client);
void clean_shutdown(int sig);
static void add_to_buffer(char* filename);

const char* argp_program_version = "ACME DummyServe v1.0 (Alphabet Animal)";
const char* argp_program_bug_address = "<bugs@example.com>";

/* Program documentation. */
static char doc[] = "ACME DummyServe -- An abstraction layer between the Dummy Processing Algorithms and Business Logic";

//settings structs
static DUMMY_SETTINGS   dummy_settings;
static bool redirect = true;

static pthread_t dsp_thread;
static struct DPROC_THREAD_CONTROL thread_control;

//the server for the socket we get the triggers and settings over
static struct sock_ev_serv server;

// Create our single-loop for this single-thread application
EV_P;

/* The options we understand. */
static struct argp_option options[] = {
  #include "dummy_settings_argp_option.c"
  { 0 }
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
  {
    #include "dummy_settings_parse_opt.c"

    case ARGP_KEY_ARG:
      return ARGP_ERR_UNKNOWN;
      
    case ARGP_KEY_END:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
   }
  return 0;
}

void clean_shutdown(int sig) {
  void* exit_status;
  
  fprintf(stderr, "Received signal %d, shutting down\n", sig);
  close(server.fd);
  ev_async_send(thread_control.EV_A, &(thread_control.cleanup));
  
  //this will block until the dsp_thread exits, at which point we will continue with out shutdown.
  pthread_join(dsp_thread, &exit_status);
  ev_loop_destroy (EV_DEFAULT_UC);
  puts("Dummy cleanup finished.");
  exit(EXIT_SUCCESS);
}

// Our argument parser.
static struct argp argp = { options, parse_opt, 0, doc };

int main (int argc, char* argv[])
{
  pthread_attr_t attr;
  int thread_status;

  dummy_settings_set_presets(&dummy_settings);

  // Parse our arguments; every option seen by parse_opt will be reflected in settings.
  argp_parse (&argp, argc, argv, 0, 0, &dummy_settings);
  
  if (true == redirect)
  {
    //these lines are to direct the stdout and stderr to log files we can access even when run as a daemon (after the possible help info is displayed.)
    //open up the files we want to use for out logs
    int new_stderr, new_stdout;
    new_stderr = open(LOG_PATH "dummy_error.log", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    new_stdout = open(LOG_PATH "dummy_debug.log", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    //truncate those files to clear the old data from long since past.
    if (0 != ftruncate(new_stderr, 0))
    {
      perror("could not truncate stderr log");
    }
    if (0 != ftruncate(new_stdout, 0))
    {
      perror("could not truncate stdout log");
    }

    //duplicate the new file descriptors and assign the file descriptors 1 and 2 (stdout and stderr) to the duplicates
    dup2(new_stderr, 2);
    dup2(new_stdout, 1);

    //now that they are duplicated we can close them and let the overhead c stuff worry about closing stdout and stderr later.
    close(new_stderr);
    close(new_stdout);
  }

  int max_queue = 128;
  struct ev_periodic every_few_seconds;
  
  EV_A = ev_default_loop(0);

  // To be sure that we aren't actually blocking
  ev_periodic_init(&every_few_seconds, not_blocked, 0, 1, 0);
  ev_periodic_start(EV_A_ &every_few_seconds);

  // Set the priority of this whole process higher (requires root)
  setpriority(PRIO_PROCESS, 0, -13); // -15

  // initialize the values of the struct that we will be giving to the new thread
  pthread_mutex_init(&(thread_control.settings_lock), NULL);
  thread_control.dummy_settings = &dummy_settings;

  pthread_mutex_init(&(thread_control.buffer_lock), NULL);
  thread_control.buffer_head = 0;
  thread_control.buffer_count = 0;

  thread_control.EV_A = ev_loop_new(EVFLAG_AUTO);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // Initialize the thread that will manage the DPROC
  thread_status = pthread_create(&dsp_thread, &attr, dummy_worker_thread, (void *)(&thread_control));
  if (0 != thread_status)
  {
    fprintf(stderr, "thread creation failed with errno %d (%s)\n", thread_status, strerror(thread_status));
    exit(EXIT_FAILURE);
  }
  pthread_attr_destroy(&attr);


  // Create unix socket in non-blocking fashion
  #ifdef TI_DPROC
    server_init(&server, "/tmp/libev-ipc-daemon.sock", max_queue);
  #else
    char socket_address[256];
    snprintf(socket_address, sizeof socket_address, "/tmp/libev-ipc-daemon.sock%d", (int)getuid());
    server_init(&server, socket_address, max_queue);
  #endif
  
  signal(SIGQUIT, clean_shutdown);
  signal(SIGTERM, clean_shutdown);
  signal(SIGINT,  clean_shutdown);
  
  // Get notified whenever the socket is ready to read
  ev_io_init(&server.io, server_cb, server.fd, EV_READ);
  ev_io_start(EV_A_ &server.io);
  
  // Run our loop, until we recieve the QUIT, TERM or INT signals, or an 'x' over the socket.
  puts("unix-socket-echo starting...\n");
  ev_loop(EV_A_ 0);

  // This point is only ever reached if the loop is manually exited
  clean_shutdown(0);
  return 0;
}

static void not_blocked(EV_P_ ev_periodic *w, int revents) {
  static int up_time = 0;
  struct stat sb;
  int status1, status2;
  
  //keep the stdout log file from getting too big and crashing the system after a long time running.
  //we are in big trouble anyway if the stderr file gets that big.
  fstat(1, &sb);
  if(sb.st_size > 1048576)
  {
    status1 = ftruncate(1, 0);
    status2 = lseek(1, 0, SEEK_SET);
    if ( (0 != status1) || (0 != status2))
    {
      perror("truncating stdout file");
    }
    else
    {
      printf("successfully truncated the stdout file\n");
    }
  }
  
  printf("I'm still alive (%d)\n", up_time);
  up_time += 1;
}

static void add_to_buffer(char* filename)
{
  int buffer_tail;

  pthread_mutex_lock(&(thread_control.buffer_lock));
  if (thread_control.buffer_count < BUFFER_SIZE)
  {
    buffer_tail = (thread_control.buffer_head + thread_control.buffer_count) % BUFFER_SIZE;
    strncpy(thread_control.buffer[buffer_tail], filename, sizeof thread_control.buffer[buffer_tail]);
    thread_control.buffer_count += 1;
    printf("added to the buffer. now contains %d\n", thread_control.buffer_count);
  }
  else
  {
    //buffer is full, overwrite the oldest entry and move the head to the next oldest one
    buffer_tail = thread_control.buffer_head;
    thread_control.buffer_head = (thread_control.buffer_head + 1) % BUFFER_SIZE;
    printf("overwriting %s in the buffer\n", thread_control.buffer[buffer_tail]);
    strncpy(thread_control.buffer[buffer_tail], filename, sizeof thread_control.buffer[buffer_tail]);
  }
  pthread_mutex_unlock(&(thread_control.buffer_lock));

  ev_async_send(thread_control.EV_A, &(thread_control.process_data));
}

int close_client(EV_P_ struct sock_ev_client* client)
{
  puts("orderly disconnect");
  ev_io_stop(EV_A_ &client->io);
  close(client->fd);
  free(client);
  return 0;
}

// This callback is called when client data is available
static void client_cb(EV_P_ ev_io *w, int revents) {
  // a client has become readable

  struct sock_ev_client* client = (struct sock_ev_client*) w;

  char filename[32];
  int n;

  if(0 == client->type)
  {
    printf("[r]");
    n = recv(client->fd, &(client->type), sizeof (client->type), 0);
    if (n <= 0) {
      if (0 == n) {
        // an orderly disconnect
        close_client(EV_A_ client);
      } else if (EAGAIN == errno) {
        fprintf(stderr, "should never get in this state (EAGAIN) with libev\n");
      } else {
        perror("recv type");
      }
      return;
    }
  }

  if ('g' == client->type) {
    puts("g - dummy_settings");
    usleep(10);

    pthread_mutex_lock(&(thread_control.settings_lock));
    n = recv(client->fd, &dummy_settings, sizeof dummy_settings, 0);
    pthread_mutex_unlock(&(thread_control.settings_lock));

    if (sizeof dummy_settings != n) {
      perror("recv dummy struct");
      return;
    }
    close_client(EV_A_ client);

    // tell the DPROC thread to copy the settings from the pointers we gave it
    ev_async_send(thread_control.EV_A, &(thread_control.update_settings));
  }
  else if ('.' == client->type)
  {
    memset(filename, 0, sizeof filename);
    puts(". - process new data (same settings)");
    usleep(10);
    n = recv(client->fd, filename, sizeof filename, 0);
    printf("received %d bytes for the filename %s\n", n, filename);
    if (n <= 0) {
      perror("recv raw data filename");
      return;
    }
    close_client(EV_A_ client);
    add_to_buffer(filename);
  }
  else if ('x' == client->type)
  {
    puts("x - exit");
    close_client(EV_A_ client);
    ev_unloop(EV_A_ EVUNLOOP_ALL);
  }
  else
  {
    fprintf(stderr, "unknown socket type. %d, or '%c' not a valid type.\nIgnoring request\n", client->type, client->type);
    close_client(EV_A_ client);
    exit(EXIT_FAILURE);
  }

  /*
  // Ping back to let the client know the message was received with success
  if (send(client->fd, ".", strlen("."), 0) < 0) {
    perror("send");
  }
  */
  puts("done with loop");
}

inline static struct sock_ev_client* client_new(int fd) {
  puts("new client");
  struct sock_ev_client* client;

  client = realloc(NULL, sizeof(struct sock_ev_client));
  client->fd = fd;
  client->type = 0;
  setnonblock(client->fd);
  ev_io_init(&client->io, client_cb, client->fd, EV_READ);

  puts(".nc");
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
        fprintf(stderr, "accept() failed errno=%i (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
      }
      break;
    }
    puts("accepted a client");
    client = client_new(client_fd);
    client->server = server;
    //client->index = array_push(&server->clients, client);
    ev_io_start(EV_A_ &client->io);
  }
  puts(".us");
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

    struct timeval timeout = {0, 500000}; // 500000 us, ie .5 seconds;

    server->fd = unix_socket_init(&server->socket, sock_path, max_queue);
    server->socket_len = sizeof(server->socket.sun_family) + strlen(server->socket.sun_path);

    //array_init(&server->clients, 128);

    if (-1 == bind(server->fd, (struct sockaddr*) &server->socket, server->socket_len))
    {
      perror("echo server bind");
      exit(EXIT_FAILURE);
    }

    if (-1 == listen(server->fd, max_queue)) {
      perror("listen");
      exit(EXIT_FAILURE);
    }
    
    if (-1 == setsockopt(server->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout)) {
      perror("setting timeout to 0.5 sec");
      exit(EXIT_FAILURE);
    }
    printf("set the receive timeout to %lf\n", ((double)timeout.tv_sec+(1.e-6)*timeout.tv_usec));
    return 0;
}
