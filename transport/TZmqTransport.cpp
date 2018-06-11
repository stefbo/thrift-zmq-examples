#include <algorithm>
#include <cassert>
#include <iostream> // remove
#include <sstream>

#include "TZmqTransport.h"

namespace apache { namespace thrift { namespace transport {

TZmqTransport::TZmqTransport(stdcxx::shared_ptr<zmq::socket_t>& sock)
    : sock_(sock) {
  if (!sock_) {
    throw TTransportException(TTransportException::BAD_ARGS,
                              "ZeroMQ socket is invalid..");
  }
}

void TZmqTransport::open() {
  // Intentionally left empty.
}

bool TZmqTransport::isOpen() {
  return true;
}

void TZmqTransport::close() {
  sock_->close();
}

uint32_t TZmqTransport::read(uint8_t* buf, uint32_t len) {
  // \todo Does not work with e.g. DEALER or multi-part PUB.

  if (rbuf_.available_read() == 0) {
    try {
      if (!sock_->recv(&inmsg_)) {
        throw TTransportException(TTransportException::TIMED_OUT);
      }
      rbuf_.resetBuffer((uint8_t*) inmsg_.data(), inmsg_.size());
    } catch (zmq::error_t& e) {
      throw TTransportException(TTransportException::UNKNOWN,
          std::string("Receiving ZeroMQ message failed. ") + e.what());
    }
  }
  return rbuf_.read(buf, len);
}

void TZmqTransport::write(const uint8_t* buf, uint32_t len) {
  wbuf_.write(buf, len);
}

void TZmqTransport::flush() {
  try {
    uint8_t* buf = NULL;
    uint32_t size = 0;
    wbuf_.getBuffer(&buf, &size);
    zmq::message_t msg(buf, size);

    // Make sure the data is flushed internally, even in case of an error.
    wbuf_.resetBuffer(true);

    if (!sock_->send(msg)) {
      throw TTransportException(TTransportException::TIMED_OUT);
    }
  } catch (zmq::error_t& e) {
    throw TTransportException(TTransportException::UNKNOWN,
        std::string("Sending ZeroMQ message failed. ") + e.what());
  }
}

stdcxx::shared_ptr<zmq::socket_t> TZmqTransport::getSocket() {
  return sock_;
}

}}}  // apache::thrift::transport
