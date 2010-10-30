#!/usr/bin/env node
(function () {
  var net = require("net"),
    stream;

  stream = net.createConnection(3333, 'localhost');
  stream.on('connect', function () {
    stream.write("hello world\n");
    stream.end();
  });
}());
