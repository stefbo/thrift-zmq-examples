
#ifndef H433B14BA_3B69_4FA3_A649_C71466B3580D
#define H433B14BA_3B69_4FA3_A649_C71466B3580D

#include "TZmqTransport.h"

#include <zmq.hpp>

#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/stdcxx.h>

namespace stdcxx = apache::thrift::stdcxx;
using apache::thrift::transport::TZmqTransport;

typedef apache::thrift::protocol::TCompactProtocol Protocol;
typedef apache::thrift::protocol::TCompactProtocolFactory ProtocolFactory;

#endif /* H433B14BA_3B69_4FA3_A649_C71466B3580D */
