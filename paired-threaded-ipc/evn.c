#include "evn.h"

#include <unistd.h> // close
#include <stdlib.h> // free

int evn_stream_destroy(EV_P_ struct evn_stream* stream)
{
  ev_io_stop(EV_A_ &stream->io);
  close(stream->fd);
  free(stream);
  return 0;
}

