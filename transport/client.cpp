#include <thrift/protocol/TCompactProtocol.h>
#include "TZmqTransport.h"

#include <zmq.hpp>

#include <iostream>
#include <memory>

#include "gen-cpp/Example.h"

using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;
using namespace std;

int main(int argc, char **argv) {
  zmq::context_t context;
  auto sock = make_shared<zmq::socket_t>(context, ZMQ_PUSH);
  sock->connect("tcp://127.0.0.1:6000");

  auto transport = make_shared<TZmqTransport>(sock);
  //auto transport = make_shared<TZmqTransport>(context, "tcp://127.0.0.1:6000", ZMQ_REQ, /*doBind=*/false);
  auto protocol = make_shared<TCompactProtocol>(transport);
  ExampleClient client(protocol);

  //cout << "3+4=" << client.add(3, 4) << endl;
  //cout << "2+3=" << client.add(2, 3) << endl;

  client.println("hello world!");

  return 0;
}
