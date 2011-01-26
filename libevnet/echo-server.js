#!/usr/bin/env node
(function () {
  "use strict"

  var net = require("net"),
    server,
    interval,
    timed_out = false;

  function on_connection(stream) {
    console.log("[Server] On Connection. [" + server.connections + "]");

    stream.setTimeout(5000);
    // stream.setNoDelay();
    // stream.setKeepAlive();

    stream.on('connection', function () {
      timeout = false;
      console.log("[Stream] On Connect");
    });

    stream.on('secure', function () {
      console.log("[Stream] On Secure");
    });

    stream.on('data', function (data) {
      timeout = false;
      console.log("[Stream] On Data");
      stream.write(data);
    });

    stream.on('end', function () {
      console.log("[Stream] On End (received FIN)");
    });

    stream.on('timeout', function () {
      console.log("[Stream] On Timeout");
      stream.emit('error', new Error("timeout"));
      // The normal choice would be to
      // close the stream, not to trigger an error
      // but, for the sake of example...
      // stream.close();
    });

    stream.on('error', function (err) {
      console.log("[Stream] On Error" + err.message);
    });

    stream.on('close', function (had_error) {
      console.log("[Stream] On Close (file descriptor closed)");
      stream.write("cause error");
    });
  }

  function on_close() {
    console.log("[Server] closing; all streams will be closed");
    clearInterval(interval);
  }

  function check_for_timeout() {
    if (true === timed_out) {
      server.close();
    }
    timed_out = true;
  }

  server = net.createServer(on_connection);
  //net.on('connection', on_connection);
  server.on('close', on_close);
  server.maxConnections = 100;
  server.listen('/tmp/libevnet-echo.' + process.getuid() + '.sock');
  //server.listen(3355, 'localhost');
  //server.listenFD(some_fd);
  console.log("[Server] listening");
  
  interval = setInterval(check_for_timeout, 5000);
}());
