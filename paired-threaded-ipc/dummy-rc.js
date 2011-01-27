#!/usr/bin/env node
(function () {
  var net = require('net'),
    stream;

  stream = net.createConnection("/tmp/dummyd-504.sock");
  stream.on("connect", function () {
    console.log("connected");
    stream.write(".Hello World!                    ");
    stream.end();
  });
}());
