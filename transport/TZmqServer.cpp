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

#include "TZmqServer.h"
#include <thrift/transport/TBufferTransports.h>

#include <iostream> // \todo remove

namespace apache { namespace thrift { namespace server {

TZmqServer::TZmqServer(
    const stdcxx::shared_ptr<TProcessor>& processor,
    const stdcxx::shared_ptr<transport::TZmqTransport>& transport,
    const stdcxx::shared_ptr<protocol::TProtocol>& protocol)
  // \todo What to do with the protocol?
    : TServer(processor),
      processor_(processor),
      transport_(transport), protocol_(protocol) {
  // Intentionally left empty.
}

void TZmqServer::serve()
{
  if (!transport_->isOpen()) {
    std::cout << "Transport not open!" << std::endl;
  }
  // \todo handle return value

  while(true) {
    processor_->process(protocol_, NULL);
  }
}

}}} // apache::thrift::server
