libevnet - Evented Networking in C
====

`libevnet` is essentially the `net` module of Node.JS, implemented in `C`

Status: working, but in very early development

API
====

net.Stream
----

  * `evn_stream* evn_create_connection(int port, char* address)` - create a Unix or TCP stream
    * If `port` is `0`, a **Unix Socket** is assumed. Otherwise a **TCP Socket** is assumed.
  * `evn_`
  * `struct evn_stream* evn_stream_create(int fd)`
  * `struct evn_stream* evn_create_connection(EV_P_ int port, char* address)`
  * `void evn_stream_priv_on_read(EV_P_ ev_io *w, int revents)`
  * `bool evn_stream_write(EV_P_ struct evn_stream* stream, void* data, int size)`
  * `bool evn_stream_end(EV_P_ struct evn_stream* stream)` - closes (and frees) the stream

**Event Callbacks**

  * `typedef void (evn_stream_on_connect)(EV_P_ struct evn_stream* stream)`
  * `typedef void (evn_stream_on_secure)(EV_P_ struct evn_stream* stream)` // TODO
  * `typedef void (evn_stream_on_data)(EV_P_ struct evn_stream* stream, void* blob, int size)`
  * `typedef void (evn_stream_on_end)(EV_P_ struct evn_stream* stream)`
  * `typedef void (evn_stream_on_timeout)(EV_P_ struct evn_stream* stream)` // TODO
  * `typedef void (evn_stream_on_drain)(EV_P_ struct evn_stream* stream)`
  * `typedef void (evn_stream_on_error)(EV_P_ struct evn_stream* stream, struct evn_exception* error)`
  * `typedef void (evn_stream_on_close)(EV_P_ struct evn_stream* stream, bool had_error)`


net.Server
----

  * `struct evn_server* evn_server_create(EV_P_ evn_server_on_connection* on_connection)`
  * `int evn_server_listen(struct evn_server* server, int port, char* address)` - create a Unix or TCP listener
    * If `port` is `0`, a **Unix Socket** is assumed. Otherwise a **TCP Socket** is assumed.
  * `int evn_server_close(EV_P_ struct evn_server* server)` -- closes (and frees) the server

**Event Callbacks**

  * `typedef void (evn_server_on_listen)(EV_P_ struct evn_server* server)`
  * `typedef void (evn_server_on_connection)(EV_P_ struct evn_server* server, struct evn_stream* stream)`
  * `typedef void (evn_server_on_close)(EV_P_ struct evn_server* server)`

TODO
====

  * Provide reference implementations in JavaScript
  * More tests and usage
  * Drop unnecessary `EV_A`s (the stream and server are created with the loop as a member.)
