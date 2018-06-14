#include "common.h"
#include "TZmqServer.h"
#include "gen-cpp/Example.h"

#include <iostream>

using apache::thrift::server::TZmqServer;
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
  stdcxx::shared_ptr<zmq::socket_t> sock (new zmq::socket_t(context, ZMQ_REP));
  sock->bind(endpoint);

  stdcxx::shared_ptr<TZmqTransport> transport(new TZmqTransport(sock));
  stdcxx::shared_ptr<ExampleHandler> handler(new ExampleHandler);
  stdcxx::shared_ptr<ExampleProcessor> processor(new ExampleProcessor(handler));
  stdcxx::shared_ptr<ProtocolFactory> protocolFactory(new ProtocolFactory);

  TZmqServer server(context, processor, transport, protocolFactory);

  cout << "Serving endpoint " << endpoint << ". Press CTRL+C to abort ..." << endl;
  server.serve();

  return 0;
}
