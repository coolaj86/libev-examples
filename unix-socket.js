#!/usr/bin/env node
(function () {
  var net = require("net"),
    stream;

  stream = net.createConnection('/tmp/libev-echo.sock');
  stream.on('connect', function () {
    stream.write("hello world\n");
  });
  stream.on('data', function (data) {
    console.log(data.toString());
    stream.end();
  });
}());
