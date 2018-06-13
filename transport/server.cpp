#include <thrift/protocol/TCompactProtocol.h>
#include "TZmqTransport.h"
#include "TZmqServer.h"

#include <zmq.hpp>

#include <iostream>
#include <memory>
#include <thread>

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
    usleep(1000*250);
  }
};

int main(int argc, char **argv) {

  if(argc != 2) {
    cerr << "Usage: " << argv[0] << " endpoint" << endl;
    return 1;
  }

  string endpoint = argv[1];
  zmq::context_t context;
  auto sock = make_shared<zmq::socket_t>(context, ZMQ_PULL);
  sock->bind(endpoint);

  auto transport = make_shared<TZmqTransport>(sock);
  auto handler = make_shared<ExampleHandler>();
  auto processor = make_shared<ExampleProcessor>(handler);

  TZmqServer server(context, processor, transport, make_shared<TCompactProtocolFactory>());

  std::thread timeoutThread([&server](){usleep(2*1000*1000); server.stop();});

  cout << "Serving endpoint " << endpoint << ". Press CTRL+C to abort ..." << endl;
  server.serve();

  timeoutThread.join();

  return 0;
}
