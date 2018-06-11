#include <thrift/protocol/TCompactProtocol.h>
#include "TZmqTransport.h"
#include "TZmqServer.h"

#include <zmq.hpp>

#include <iostream>
#include <memory>

#include "gen-cpp/Example.h"

using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace std;

class ExampleHandler : public ExampleIf
{
public:
  virtual double add(const double arg1, const double arg2) {
    cout << "add" << endl;
    return arg1+arg2;
  }

  virtual void println(const std::string& msg) {
    cout << "\"" << msg << "\"" << endl;
  }
};

int main(int argc, char **argv) {
  zmq::context_t context;
#if 1
  auto sock = make_shared<zmq::socket_t>(context, ZMQ_PULL);
  sock->bind("tcp://*:6000");

  auto transport = make_shared<TZmqTransport>(sock);
#else
  auto transport = make_shared<TZmqTransport>(context, "tcp://*:6000", ZMQ_REP, /*doBind=*/true);
#endif
  auto protocol = make_shared<TCompactProtocol>(transport);
  auto handler = make_shared<ExampleHandler>();
  auto processor = make_shared<ExampleProcessor>(handler);

  TZmqServer server(processor, transport, protocol);
  server.serve();

  return 0;
}
