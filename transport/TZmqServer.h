/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _THRIFT_SERVER_TZMQSERVER_H_
#define _THRIFT_SERVER_TZMQSERVER_H_ 1

#include <zmq.hpp>
#include <thrift/server/TServer.h>
#include "TZmqTransport.h"

namespace apache { namespace thrift { namespace server {

/// @note Does not support per-client processors.
///
/// @note Events handlers `TServerEventHandler` are currently not supported.
///
class TZmqServer : public TServer {
 public:
  TZmqServer(const stdcxx::shared_ptr<TProcessor>& processor,
             const stdcxx::shared_ptr<transport::TZmqTransport>& transport,
             const stdcxx::shared_ptr<protocol::TProtocol>& protocol);

  void serve();

 private:
  // The `TServer` stores the processor using a `TSingletonProcessorFactory`,
  // but the lookup is rather complicated. See `TServer::getProcessor`.
  stdcxx::shared_ptr<TProcessor> processor_;

  // The transport cannot be stored in `TServer`, because it does only accept
  // `TServerTransport`.
  stdcxx::shared_ptr<TTransport> transport_;

  stdcxx::shared_ptr<protocol::TProtocol> protocol_;
};

}}} // apache::thrift::server

#endif // #ifndef _THRIFT_SERVER_TZMQSERVER_H_
