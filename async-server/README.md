This example demonstrates how to use [Apache Thrift](https://github.com/apache/thrift) and [ZeroMQ](http://zeromq.org/) to write a REQ/REP asynchronous server.

Overview
========

The examples has the following features:
- A `TrivialCooperativeScheduler` which executes `Task`s.
- A server application which implements an Thrift service. Requests are either processed asynchronously using `Task`s or synchronously in the service's handler.
- A synchronous client which uses the Thrift service.
- The async. server is based on ZeroMQ's ROUTER/DEALER socker combination and Thrift's COB style handlers.

Motivation
==========

While Apache Thrift is a good choice when using basic communication styles like synchronous request/reply, but it falls short when it comes to more advanced styles like asynchronous clients and servers. At the current time (Jun. 2018), the only async. server provided by Apache Thrift uses HTTP and `libevent`, which is not the best choice when doing low latency communication with embedded devices. On the other hand, ZeroMQ supports a whole bunch of communication styles including async. REQ/REP servers using `ROUTER` sockets, but leaves the message definition open. By using both libraries hand in hand, we get the both from both worlds. IDL, code generation and dispatching from Apache Thrift and advanced communication styles from ZeroMQ.

Build
=====

Use the provided CMake file to build the examples:

    $ mkdir build
    $ cd build
    $ make
