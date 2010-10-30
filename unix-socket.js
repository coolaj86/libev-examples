#!/usr/bin/env node
(function () {
  var net = require("net"),
    stream;

  stream = net.createConnection('/tmp/libev_echo.sock');
  stream.on('connect', function () {
    stream.write("hello world\n");
    stream.end();
  });
}());
