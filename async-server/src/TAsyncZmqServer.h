/*
 * This file is part of the Thrift-ZeroMQ examples
 *    (https://github.com/stefbo/thrift-zmq-examples).
 * Copyright (c) 2018, Stefan Bolus.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef H8FE73F63_FB2C_4C0D_99DE_923F55134B11
#define H8FE73F63_FB2C_4C0D_99DE_923F55134B11

#include <zmq.hpp>
#include <thrift/async/TAsyncBufferProcessor.h>

#include <thread>
#include <map>
#include <memory>

//// Asynchronous Apache Thrift server using ZeroMQ
class TAsyncZmqServer {
public:
  /// Constructor.
  ///
  /// @param processor The asynchronous protocol processor.
  /// @param context The ZeroMQ context. Must not be `nullptr`.
  /// @param sock The socket of type `ZMQ_ROUTER`. It is up to the called to
  /// connect the socket or bind it to an endpoint.
  TAsyncZmqServer(
      std::shared_ptr<apache::thrift::async::TAsyncBufferProcessor> processor,
      zmq::context_t * context, std::shared_ptr<zmq::socket_t> sock);
  ~TAsyncZmqServer();

  /// Starts serving requests.
  ///
  /// Blocks until the server is stoped by calling @ref stop.
  void serve();

  /// Signals the server to stop processing new requests and any pending
  /// responses.
  ///
  /// The server may take a moment to being stopped. Use @ref waitStopped
  /// to block a thread until the server stopped.
  ///
  /// @pre getState() == RUNNING
  /// @post getState() in {STOPPING, STOPPED}
  void stop();

  /// Blocks the current thread until the server is stopped.
  ///
  /// @return Returns `false` if waiting failed.
  bool waitStopped();

private:
  class RequestContext;
  class MultipartMessage;

  enum State {
    INIT,
    RUNNING,
    STOPPING,
    STOPPED
  };

  State state_;
  std::shared_ptr<apache::thrift::async::TAsyncBufferProcessor> processor_;
  zmq::context_t * context_;
  std::shared_ptr<zmq::socket_t> sock_;
  zmq::socket_t dealer_; // Used to transport messages from other threads to the router's one.
  std::pair<zmq::socket_t,zmq::socket_t> stop_signal_; // Used to internally control the server
  std::thread::id thread_id_;

  void init();
  void createChannelForCurrentThread();
  zmq::socket_t & getChannelForCurrentThread();
  void process(MultipartMessage && msg);
  void complete(std::shared_ptr<RequestContext> ctx, bool success) throw();

  std::map<std::thread::id, std::shared_ptr<zmq::socket_t>> channels_;
};

#endif /* H8FE73F63_FB2C_4C0D_99DE_923F55134B11 */
