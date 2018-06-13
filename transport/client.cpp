#include <thrift/protocol/TCompactProtocol.h>
#include "TZmqTransport.h"

#include <zmq.hpp>

#include <iostream>
#include <memory>
#include <chrono>

#include "gen-cpp/Example.h"

using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;
using namespace std;

int main(int argc, char **argv) {
  if(argc != 2) {
    cout << "Usage: " << argv[0] << " endpoint" << endl;
    return 1;
  }

  string endpoint = argv[1];

  zmq::context_t context;
  auto sock = make_shared<zmq::socket_t>(context, ZMQ_PUSH);
  sock->connect(endpoint);

  auto transport = make_shared<TZmqTransport>(sock);
  //auto transport = make_shared<TZmqTransport>(context, "tcp://127.0.0.1:6000", ZMQ_REQ, /*doBind=*/false);
  auto protocol = make_shared<TCompactProtocol>(transport);
  ExampleClient client(protocol);

  //cout << "3+4=" << client.add(3, 4) << endl;
  //cout << "2+3=" << client.add(2, 3) << endl;

  auto t = std::chrono::high_resolution_clock::now();
  client.println("hello world!");
  client.println("hello world!");
  client.println("hello world!");
  client.println("hello world!");
  client.println("hello world!");
  auto elapsed = std::chrono::high_resolution_clock::now() - t;
  cout << "Elpased: " << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() << endl;

  return 0;
}
