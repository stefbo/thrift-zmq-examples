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

#include "TAsyncZmqServer.h"
#include <thrift/async/TAsyncBufferProcessor.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/TLogging.h>

#include <iostream>
#include <iterator>

using apache::thrift::async::TAsyncBufferProcessor;
using apache::thrift::transport::TMemoryBuffer;

using namespace std;

/// Handles multi-part messages for REQ/REP pattern.
///
/// Frames until the empty frame are considered address frames. The frame
/// afterwards contains the actual data. Only a single frame containing data
/// is expected.
class TAsyncZmqServer::MultipartMessage
{
 public:
  MultipartMessage() = default;

  // The messages are reference counted.
  MultipartMessage(MultipartMessage&& other) { msgs_.swap(other.msgs_); }
  MultipartMessage& operator=(MultipartMessage&) = delete;
  MultipartMessage(MultipartMessage&) = delete;

  /// Receive the multi-part message from a socket.
  ///
  /// Any new messages received from the socket are appended to the existing
  /// messages. In case of an error (`EAGAIN` is not an error), no part of the
  /// multi-part message is kept.
  ///
  /// @return Returns `false` only if ZeroMQ's `zmq_recv` failed with `EAGAIN`.
  /// @throws Throws `zmq::error_t` on error.
  bool receive(zmq::socket_t & sock) {
    bool okay = true;
    int64_t has_more = 1;
    std::vector<zmq::message_t> tmp;

    while (has_more && okay) {
      zmq::message_t msg;
      okay = sock.recv(&msg);
      if (okay) {
        has_more = sock.getsockopt<int64_t>(ZMQ_RCVMORE);
        tmp.emplace_back(std::move(msg));
      }
    }

    msgs_.insert(msgs_.end(), std::make_move_iterator(tmp.begin()),
                 std::make_move_iterator(tmp.end()));
    return okay;
  }

  /// Send the multi-part message to the socket.
  ///
  /// @return Returns `false` only if ZeroMQ's `zmq_send` failed with `EAGAIN`.
  /// @throws Throws `zmq::error_t` on error.
  bool send(zmq::socket_t & sock) {

    bool okay = true;
    auto iter = msgs_.begin();
    for (; iter != msgs_.end() && okay; ++iter) {
      auto & msg = *iter;
      const bool has_more = (iter + 1 != msgs_.end());

      int flags = 0;
      if (has_more) {
        flags = ZMQ_SNDMORE;
      }
      okay = sock.send(msg, flags);
    }

    // Sending messages transfers ownership of the messages to ZeroMQ.
    msgs_.erase(msgs_.begin(), iter);
    return okay;
  }

  /// Returns the single message that contains the data (if any). Returns
  /// `nullptr` otherwise.
  zmq::message_t * getDataFrame() {
    zmq::message_t * result = nullptr;
    if (!msgs_.empty()) {
      result = &msgs_.back();
    }
    return result;
  }

  /// Sets data of the multi-part message's data frame to the data of the given
  /// `TMemoryBuffer`.
  void setData(TMemoryBuffer & obuf) {
    uint8_t* ptr = nullptr;
    uint32_t size = 0;
    obuf.getBuffer(&ptr, &size);

    setData(ptr, size);
  }

 private:
  /// Sets the data of the multi-part message's data frame.
  void setData(uint8_t * buf, size_t size) {
    assert(!msgs_.empty());
    assert(buf != NULL);
    assert(size >= 0);

    msgs_.back().rebuild(buf, size);
  }

  std::vector<zmq::message_t> msgs_;
};

class TAsyncZmqServer::RequestContext {
 public:
  MultipartMessage msg;
  shared_ptr<TMemoryBuffer> ibuf;
  shared_ptr<TMemoryBuffer> obuf;

  // Takes ownership of the message.
  RequestContext(MultipartMessage && msgArg);
};

TAsyncZmqServer::TAsyncZmqServer(shared_ptr<TAsyncBufferProcessor> processor, zmq::context_t * context, std::shared_ptr<zmq::socket_t> sock)
    : processor_(processor),
      context_(context),
      sock_(sock),
      dealer_(*context, ZMQ_DEALER),
      stop_signal_({zmq::socket_t(*context, ZMQ_PAIR), zmq::socket_t(*context, ZMQ_PAIR)}) {
  init();
}

void TAsyncZmqServer::init() {
  assert(sock_);
  assert(sock_->getsockopt<int>(ZMQ_TYPE) == ZMQ_ROUTER);

  state_ = INIT;
  dealer_.bind("inproc://TAsyncZmqServer_internal");
  stop_signal_.second.bind("inproc://TAsyncZmqServer_stop");
  stop_signal_.first.connect("inproc://TAsyncZmqServer_stop");
}

TAsyncZmqServer::~TAsyncZmqServer() {
  stop();
  waitStopped();
}

void TAsyncZmqServer::serve() {
  thread_id_ = std::this_thread::get_id();
  channels_[thread_id_] = sock_;

  state_ = RUNNING;

  T_DEBUG_L(1, "Server running.");

  while (RUNNING == state_) {

    zmq_pollitem_t items[] = { { *sock_, 0, ZMQ_POLLIN, 0 }, { dealer_, 0,
        ZMQ_POLLIN, 0 }, { stop_signal_.second, 0, ZMQ_POLLIN, 0 } };

    // Note: We currently do no handle EGAIN here.
    // Note: If any IO operation fails, we simple silently drop the message.

    int ret = zmq::poll(items, /*nitems=*/3, std::chrono::milliseconds(1000));
    if (ret > 0) {
      if (items[0].revents & ZMQ_POLLIN) {
        MultipartMessage msg;
        (void)msg.receive(*sock_);
        process(move(msg));
      }

      if (items[1].revents & ZMQ_POLLIN) {
        MultipartMessage msg;
        (void)msg.receive(dealer_);
        (void)msg.send(*sock_);
      }

      if (items[2].revents & ZMQ_POLLIN) {
        T_DEBUG_L(1, "Server received STOP signal.");
        break;
      }
    }
  }

  state_ = STOPPED;
  T_DEBUG_L(1, "Server stopped.");

  // Send back, so someone can synchronize on the STOPPING->STOPPED transition.
  stop_signal_.second.send("", 0);
}

TAsyncZmqServer::RequestContext::RequestContext(MultipartMessage && msgArg)
  : msg(move(msgArg)),
    ibuf(new TMemoryBuffer((uint8_t*)msg.getDataFrame()->data(), msg.getDataFrame()->size())),
    obuf(new TMemoryBuffer()) {
  // Intentionally left empty.
}

void TAsyncZmqServer::process(MultipartMessage && msg) {
  auto rq = make_shared<RequestContext>(move(msg));
  return processor_->process(
      std::bind(&TAsyncZmqServer::complete, this, rq, std::placeholders::_1),
      rq->ibuf, rq->obuf);
}

// Let user of the server add logic to pre-process the message, e.g. transfer
// it between threads. Idea: `complete` may be called from any thread, but the
// custom-function guarantees that the message is sent from the socket's thread.

void TAsyncZmqServer::createChannelForCurrentThread()
{
  T_DEBUG_L(1, "Creating additional socket for internal communication with thread %ul ...", this_thread::get_id());

  auto s = std::make_shared<zmq::socket_t>(*context_, ZMQ_DEALER);
  s->connect("inproc://TAsyncZmqServer_internal");
  channels_[this_thread::get_id()] = s;
}


// Throws std::out_of_range if the no socket could be created.
zmq::socket_t & TAsyncZmqServer::getChannelForCurrentThread()
{
  auto key = this_thread::get_id();
  auto iter = channels_.find(key);
  if(iter == channels_.end()) {
    createChannelForCurrentThread();
  }

  return *channels_.at(key);
}

void TAsyncZmqServer::complete(shared_ptr<RequestContext> rq, bool success) throw() {
  // Indirectly called by the continuation object passed to the async. service
  // handler. Hence, it may be called from any arbitrary threads in our case.

  if (!success) {
    // Note: `success` is set by `TAsyncDispatchProcessor` if there are errors in
    // the Thrift protocol layer, e.g. if the header contains invalid information.
    T_ERROR("Received malformed Thrift request. Ignored!");
  } else {
    try {
      auto & dest_sock = getChannelForCurrentThread();

      rq->msg.setData(*rq->obuf);
      //rq->msg.print();
      (void) rq->msg.send(dest_sock);
    } catch (...) {
      // Ignore.
      T_ERROR("Error when sending.");
    }
  }
}

void TAsyncZmqServer::stop() {
  if(RUNNING == state_) {
    state_ = STOPPING;

    cout << "Sending signal to stop server ... ";
    cout.flush();
    (void)zmq_send(stop_signal_.first, "", 0, 0);
    cout << "done" << endl;
  }
}

bool TAsyncZmqServer::waitStopped()
{
  if (state_ != STOPPED) {
    try {
      char buf[1];
      (void) stop_signal_.first.recv(buf, sizeof(buf));
    } catch (zmq::error_t & e) {
      T_ERROR("Waiting for server stopped failed. %s", e.what());
    }
  }

  return STOPPED == state_;
}
