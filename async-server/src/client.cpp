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

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <chrono>
#include <thrift/protocol/TCompactProtocol.h>

#include "zmq.hpp"
#include "TZmqClient.h"
#include "gen-cpp/Service.h"

using apache::thrift::stdcxx::shared_ptr;
using apache::thrift::transport::TZmqClient;
using apache::thrift::protocol::TCompactProtocol;

using namespace example;
using namespace std;
using namespace std::chrono;

namespace {

/// Timer to measure elapsed time.
class Timer
{
 public:
  Timer() { restart(); }
  void restart() { started_ = steady_clock::now(); }
  double elapsedSecs() const { return duration_cast<microseconds>(steady_clock::now() - started_).count() / 1000000.0; }

 private:
  steady_clock::time_point started_;
};

}  // anonymous namespace

int main(int argc, char** argv) {
  const char* endpoint = "tcp://127.0.0.1:5555";

  if(argc >= 2) {
    endpoint = argv[1];
  }

  zmq::context_t ctx;
  auto transport = make_shared<TZmqClient>(ctx, endpoint, ZMQ_REQ);
  auto protocol = make_shared<TCompactProtocol>(transport);
  ServiceClient client(protocol);
  transport->open();

  for (int i = 0; i < 1000; ++i) {
    try {
      Timer t;
      cout << "Sleep: " << client.sleep(50/*ms*/);
      cout << " [took " << t.elapsedSecs() << " s]" << endl;

      t.restart();
      cout << "1+2: " << client.math(example::MathOp::MATHOP_ADD, 1, 2);
      auto elapsedMsecs = t.elapsedSecs() * 1000;
      cout << " [took " << std::fixed << elapsedMsecs << " ms]" << endl;
      cout << "2/0: " << client.math(example::MathOp::MATHOP_DIV, 2, 0) << endl;
    } catch (Error & e) {
      cout << "Caught exception from service. Code " << e.errorCode
           << ", reason: " << e.errorMessage << endl;
    }
  }

  return 0;
}
