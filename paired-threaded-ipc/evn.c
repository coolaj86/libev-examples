#include "evn.h"

#include <unistd.h> // close
#include <stdlib.h> // free

int evn_stream_destroy(EV_P_ struct evn_stream* stream)
{
  // TODO delay freeing of server until streams have closed
  // or link loop directly to stream?
  ev_io_stop(EV_A_ &stream->io);
  close(stream->fd);
  free(stream);
  return 0;
}

