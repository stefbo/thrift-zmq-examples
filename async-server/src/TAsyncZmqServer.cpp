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
#include <algorithm>
#include <cassert>

using apache::thrift::async::TAsyncBufferProcessor;
using apache::thrift::transport::TMemoryBuffer;

using namespace std;

namespace {
const std::string kStopSignal = "STOP";
}  // anonymous namespace

struct TAsyncZmqServer::RequestContext {
  zmq::multipart_t msg;
  shared_ptr<TMemoryBuffer> ibuf;
  shared_ptr<TMemoryBuffer> obuf;

  // Takes ownership of the message.
  RequestContext(zmq::multipart_t && msgArg);
};

TAsyncZmqServer::TAsyncZmqServer(shared_ptr<TAsyncBufferProcessor> processor, zmq::context_t * context, std::shared_ptr<zmq::socket_t> sock)
    : processor_(processor),
      context_(context),
      dealer_(*context, ZMQ_DEALER),
      stop_signal_({zmq::socket_t(*context, ZMQ_PAIR), zmq::socket_t(*context, ZMQ_PAIR)}) {
  init();
  addSocket(sock);
}

void TAsyncZmqServer::init() {
  state_ = INIT;
  dealer_.bind("inproc://TAsyncZmqServer_internal");
  stop_signal_.second.bind("inproc://TAsyncZmqServer_signals");
  stop_signal_.first.connect("inproc://TAsyncZmqServer_signals");
}

TAsyncZmqServer::~TAsyncZmqServer() {
  stop();
  waitStopped();
}

void TAsyncZmqServer::serve() {
  thread_id_ = std::this_thread::get_id();

  state_ = RUNNING;

  T_DEBUG_L(1, "Server running.");

  std::vector<zmq_pollitem_t> items;
  items.push_back({ dealer_, 0, ZMQ_POLLIN, 0 });
  items.push_back({ stop_signal_.second, 0, ZMQ_POLLIN, 0 });
  for (auto & sock : socks_) {
    items.push_back( { *sock, 0, ZMQ_POLLIN, 0 });
  }

  while (RUNNING == state_) {
    // Reset events.
    std::for_each(items.begin(), items.end(), [](zmq_pollitem_t & item){ item.revents = 0; });

    // Note: We currently do no handle EGAIN here.
    // Note: If any IO operation fails, we simple silently drop the message.

    int ret = zmq::poll(items, std::chrono::milliseconds(1000));
    if (ret > 0) {
      if (items[0].revents & ZMQ_POLLIN) {
        zmq::multipart_t msg;
        (void)msg.recv(dealer_);

        auto index = msg.poptyp<size_t>();
        assert(index < socks_.size());
        (void)msg.send(*socks_[index]);
      }

      if (items[1].revents & ZMQ_POLLIN) {
        T_DEBUG_L(1, "Server received STOP signal.");
        break;
      }

      for (size_t i = 2; i < items.size(); ++i) {
        if (items[i].revents & ZMQ_POLLIN) {
          size_t index = i-2;
          zmq::multipart_t msg;
          (void) msg.recv(*socks_[index]);

          // Store the sock's index for reference when sending the response.
          msg.pushtyp(index);
          process(move(msg));
        }
      }
    }
  }

  state_ = STOPPED;
  T_DEBUG_L(1, "Server stopped.");

  // Send back, so someone can synchronize on the STOPPING->STOPPED transition.
  stop_signal_.second.send("", 0);
}

TAsyncZmqServer::RequestContext::RequestContext(zmq::multipart_t && msgArg)
  : msg(move(msgArg)),
    ibuf(new TMemoryBuffer((uint8_t*)msg.rbegin()->data(), msg.rbegin()->size())),
    obuf(new TMemoryBuffer()) {
  // Intentionally left empty.
}

void TAsyncZmqServer::process(zmq::multipart_t && msg) {
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
std::shared_ptr<zmq::socket_t> TAsyncZmqServer::getChannelForCurrentThread()
{
  const auto key = this_thread::get_id();
  auto iter = channels_.find(key);
  if (iter == channels_.end()) {
    createChannelForCurrentThread();
  }

  return channels_.at(key);
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
      std::shared_ptr<zmq::socket_t> dest_sock;
      if (this_thread::get_id() == thread_id_) {
        auto index = rq->msg.poptyp<size_t>();
        assert(index < socks_.size());
        dest_sock = socks_[index];
      } else {
        dest_sock = getChannelForCurrentThread();
      }

      assert(dest_sock);

      // Replace the data in the original message.
      uint8_t* ptr = nullptr;
      uint32_t size = 0;
      rq->obuf->getBuffer(&ptr, &size);

      rq->msg.rbegin()->rebuild(ptr, size);
      (void) rq->msg.send(*dest_sock);
    } catch (...) {
      // Ignore.
      T_ERROR("Error when sending.");
    }
  }
}

void TAsyncZmqServer::stop() {
  if(RUNNING == state_) {
    state_ = STOPPING;

    T_DEBUG_L(1, "Sending signal to stop server ... ");
    stop_signal_.first.send(kStopSignal.c_str(), kStopSignal.size());
  }
}

bool TAsyncZmqServer::waitStopped() {
  if (state_ != STOPPED) {
    try {
      T_DEBUG_L(1, "Server did not stop yet. Waiting ...");
      zmq_pollitem_t item( { stop_signal_.first, 0, ZMQ_POLLIN, 0 });
      (void) zmq::poll(&item, 1);
    } catch (zmq::error_t & e) {
      T_ERROR("Waiting for server stopped failed. %s", e.what());
    }
  }

  return STOPPED == state_;
}

void TAsyncZmqServer::addSocket(std::shared_ptr<zmq::socket_t> sock) {
  if (INIT == state_) {
    assert(sock->getsockopt<int>(ZMQ_TYPE) == ZMQ_ROUTER);

    if (std::find(socks_.begin(), socks_.end(), sock) == socks_.end()) {
      socks_.push_back(sock);
    }
  }
}
