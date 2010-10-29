#!/usr/bin/env node
(function () {
  var dgram = require('dgram'),
    message = new Buffer("Some bytes"),
    client = dgram.createSocket("udp4");

  client.send(message, 0, message.length, 3333, "localhost");
  client.close();
}());
