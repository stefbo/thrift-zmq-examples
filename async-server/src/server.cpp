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

#include "gen-cpp/Service.h"
#include "TAsyncZmqServer.h"
#include "TrivialCooperativeScheduler.h"

#include <zmq.hpp>
#include <thrift/Thrift.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/async/TAsyncProtocolProcessor.h>

#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace std;
using namespace std::chrono;
using namespace example;

using apache::thrift::TDelayedException;

namespace {

// Sleeps for the user supplied amount of time. Returns the
// time spend in the cooperative scheduler (in msecs).
class SleepTask : public TaskFromRemote<int>
{
 public:
  SleepTask(
      cob_type cob, exn_cob_type exn_cob, chrono::milliseconds time_to_sleep)
      : TaskFromRemote<int>(cob, exn_cob),
        time_(time_to_sleep){
    // Intentionally left empty.
  }

  virtual bool doProgress() override
  {
    auto time_asleap = duration_cast<milliseconds>(steady_clock::now() - startTime_);

    if(time_asleap - time_ > milliseconds(0)) {
      cob_(time_asleap.count());
      return true;
    }
    else {
      return false;
    }
  }

 private:
  chrono::milliseconds time_;
};

class ServiceAsyncHandler : public ServiceCobSvIf {
 public:
  ServiceAsyncHandler(shared_ptr<TrivialCooperativeScheduler> sched) : sched_(sched) {
    // Intentionally left empty.
  }

  virtual void math(
      function<void(double const& _return)> cob,
      function<void(::apache::thrift::TDelayedException* _throw)> exn_cob,
      const example::MathOp::type op, const double arg1, const double arg2)
          override {

    // Math operations can be computed in the IO thread without too much
    // delay of other tasks.

    switch (op) {
      case example::MathOp::MATHOP_ADD:
        cob(arg1 + arg2);
        break;
      case example::MathOp::MATHOP_DIV:
        if (arg2 != .0) {
          cob(arg1 / arg2);
        } else {
          example::Error e;
          e.errorCode = example::ErrorCode::SERVICE_CMD_INVALID_ARGUMENTS;
          e.__set_errorMessage("Division by 0.");
          exn_cob(::apache::thrift::TDelayedException::delayException(e));
        }
        break;
      default:
        example::Error e;
        e.errorCode = example::ErrorCode::SERVICE_CMD_INVALID_ARGUMENTS;
        e.__set_errorMessage("Unknown math op.");
        exn_cob(::apache::thrift::TDelayedException::delayException(e));
        break;
    }
  }

  virtual void sleep(
      ::apache::thrift::stdcxx::function<void(int32_t const& _return)> cob,
      ::apache::thrift::stdcxx::function<
          void(::apache::thrift::TDelayedException* _throw)> exn_cob,
      const int32_t timeMsecs) {
    sched_->addTask(make_shared<SleepTask>(cob, exn_cob, milliseconds(timeMsecs)));
  }

 private:
  shared_ptr<TrivialCooperativeScheduler> sched_;
};

shared_ptr<TrivialCooperativeScheduler> g_sched;
shared_ptr<TAsyncZmqServer> g_server;

void handleControlC(int sig) {
  cout << "CTRL+C pressed. Stopping scheduler and server ..." << endl;
  if (g_sched)
    g_sched->stop();
}

void runServer(shared_ptr<TrivialCooperativeScheduler> sched, zmq::context_t & context)
{
  auto protocolFactory = make_shared<TCompactProtocolFactory>();
  auto handler = make_shared<ServiceAsyncHandler>(sched);
  auto dispatchProcessor = make_shared<ServiceAsyncProcessor>(handler);
  auto protocolProcessor = make_shared<TAsyncProtocolProcessor>(
      dispatchProcessor, protocolFactory);

  try {
    auto sock = make_shared<zmq::socket_t>(context, ZMQ_ROUTER);
    sock->bind("tcp://*:5555");

    g_server = make_shared<TAsyncZmqServer>(protocolProcessor, &context, sock);

    // Create another IPC socket.
    sock = make_shared<zmq::socket_t>(context, ZMQ_ROUTER);
    sock->bind("ipc:///tmp/conn");
    g_server->addSocket(sock);

    cout << "Serving requests ..." << endl;
    g_server->serve();

  } catch (zmq::error_t & e) {
    cerr << "Transport level error. " << e.what() << endl;
  }
}

}  // anonymous namespace

int main(int argc, char **argv) {
  g_sched.reset(new TrivialCooperativeScheduler);

  (void)signal(SIGINT, handleControlC);

  zmq::context_t context;

  cout << "Started server thread. Exit with CTRL+C ..." << endl;
  std::thread server_thread([&context](){ runServer(g_sched, context); });

  cout << "Started scheduler." << endl;
  g_sched->start();

  // Wait a moment, so that the server can process any pending messages.
  this_thread::sleep_for(milliseconds(500));

  cout << "Destroying server." << endl;
  g_server.reset();

  cout << "Waiting for server thread ..." << endl;
  server_thread.join();

  return 0;
}
