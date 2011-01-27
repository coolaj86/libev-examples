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

    console.log("\t[Stream] readyState: " + stream.readyState);

    stream.on('connect', function () {
      timeout = false;
      console.log("\t[Stream] On Connect");
    });

    stream.on('secure', function () {
      console.log("\t[Stream] On Secure");
    });

    stream.on('data', function (data) {
      timeout = false;
      console.log("\t[Stream] On Data");
      stream.write(data);
    });

    stream.on('end', function () {
      console.log("\t[Stream] On End (received FIN).\n\t\treadyState: " + stream.readyState);
    });

    stream.on('timeout', function () {
      console.log("\t[Stream] On Timeout");
      stream.end();
      console.log("\t[Stream] Closing (sent FIN).\n\t\treadyState: " + stream.readyState);
    });

    stream.on('drain', function () {
      console.log("\t[Stream] On Drain");
    });

    stream.on('error', function (err) {
      // not used in this example
      console.log("\t[Stream] On Error" + err.message);
    });

    stream.on('close', function (had_error) {
      console.log("\t[Stream] On Close (file descriptor closed). State: " + stream.readyState);
      // 'closed', 'open', 'opening', 'readOnly', or 'writeOnly'
      if ('open' === stream.readyState) {
        stream.write("cause error");
      }
    });
  }

  function on_close() {
    console.log("[Server] closing; waiting for all streams to close");
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
