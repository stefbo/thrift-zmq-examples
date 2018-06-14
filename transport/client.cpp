#include "common.h"
#include "gen-cpp/Example.h"

#include <iostream>

using namespace std;

int main(int argc, char **argv) {
  if(argc != 2) {
    cout << "Usage: " << argv[0] << " endpoint" << endl;
    return 1;
  }

  string endpoint = argv[1];

  zmq::context_t context;
  stdcxx::shared_ptr<zmq::socket_t> sock(new zmq::socket_t(context, ZMQ_REQ));
  sock->connect(endpoint);

  stdcxx::shared_ptr<TZmqTransport> transport(new TZmqTransport(sock));
  stdcxx::shared_ptr<Protocol> protocol(new Protocol(transport));
  ExampleClient client(protocol);

  cout << "3+4=" << client.add(3, 4) << endl;
  cout << "2+3=" << client.add(2, 3) << endl;

  //client.println("hello world!");

  return 0;
}
