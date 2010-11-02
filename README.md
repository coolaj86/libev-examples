libev Echo Client / Server
====

A collection of simple examples for libev.

Please fork and contribute.

Usage
====

The working examples so far consist of `Unix Socket` **echo server** *and* **echo client**.

unix socket
----

I'm awaiting confirmation from veterans on the **libev mailing list** that this is done, in fact, *the right way*(TM).

These should be very easily adaptable with some very very simple cut/paste action to produce a TCP example echo server.

You should be able to make them and then type on the console and see that the data does go back and forth.

    make unix-echo-server unix-echo-client

In terminal A:

    ./unix-echo-server

Then type away in terminal B:

    ./unix-echo-client

Note: Opening the client and server in reverse order won't work.

For testing and for reference, compatible servers written for `Node.JS` are provided.

udp socket
----

If I understand it correctly, it may be that This example is not actually taking
advantage of any of the features of libev.
I grabbed it from the mailing list and I think it just wraps blocking code in libev.

  * `make`the `udp-echo` binary
  * start the `udp-echo` server in one terminal
  * test that you can see the open port `3333` in another
  * test that you see when `udp.js` connets by the message in the first terminal

Expected procedure and results:

    $ make udp-echo && ./udp-echo
    cc -Wall -Werror -lev -o udp-echo udp-echo.c
    udp_echo server started...


    $ netstat -lau | grep 3333
    udp        0      0 *:3333                  *:*

    $ node udp.js
    # this show up on the server window
    udp socket has become readable

Installing libev
====

    sudo apt-get install libev libev-dev

