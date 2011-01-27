#!/usr/bin/env node
(function () {
  "use strict";

  var net = require("net"),
    client;

  client = net.createConnection('/tmp/libevnet-echo.' + process.getuid() + '.sock');
  // client = net.createConnection(3355, 'localhost');

  client.on('connect', function () {
    console.log("[Client] On Connection");
    client.write("Hello Server");
  });

  client.on('secure', function () {
    // not used in this example
    console.log("[Client] On Secure");
  });

  client.on('data', function (data) {
    console.log("[Client] On Data");
  });

  client.on('end', function () {
    console.log("[Client] On End (received FIN). readyState: " + client.readyState);
  });

  client.on('timeout', function () {
    // not used in this example
    console.log("[Client] On Timeout");
  });

  client.on('drain', function () {
    console.log("[Client] On Drain");
  });

  client.on('error', function (err) {
    console.log("[Client] On Error" + err.message);
  });

  client.on('close', function () {
    console.log("[Client] On Close. readyState: " + client.readyState);
  });
}());
