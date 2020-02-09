/* Copyright 2020 Istio Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "istio/extension/node_info/node_info.h"
#include "istio/extension/stream_info/stream_info.h"
#include "istio/extension/stream_info/stream_info.pb.h"

namespace Istio {
namespace Extension {

class ExtensionRootContext : public RootContext {
public:
  // Gets peer node info. It checks the node info cache first, and then try to
  // fetch it from host if cache miss. If cache is disabled, it will fetch from
  // host directly.
  const istio::extension::NodeInfo &getPeerNodeInfo(bool is_outbound);

  // Get Local node information.
  const istio::extension::NodeInfo &getLocalNodeInfo();

private:
  NodeInfo::NodeInfo node_info_;
};

class ExtensionStreamContext : public Context, public StreamInfo::StreamInfo {
public:
  ExtensionStreamContext(uint32_t id, RootContext *root);
  ~ExtensionStreamContext();

protected:
  /************************
       Node Property
  ************************/
  const istio::extension::NodeInfo &getSourceNodeInfo();
  const istio::extension::NodeInfo &getDestinationNodeInfo();

  /************************
      Request Property
  ************************/

  // Time
  int64_t requestTimestamp() override;
  int64_t responseTimestamp() override;
  int64_t duration() override;
  int64_t responseDuration() override;

  // Size
  int64_t requestSize() override;
  int64_t responseSize() override;

  // Connection
  const std::string &sourceAddress() override;
  const std::string &destinationAddress() override;
  int64_t destinationPort() override;
  const std::string &responseFlag() override;

  // HTTP
  const std::string &requestProtocol() override;
  int64_t responseCode() override;
  const std::string &destinationServiceName() override;
  const std::string &destinationServiceHost() override;
  const std::string &requestOperation() override;
  const std::string &urlPath() override;
  const std::string &requestHost() override;
  const std::string &requestScheme() override;

  // Auth
  ::Istio::Extension::StreamInfo::ServiceAuthenticationPolicy
  serviceAuthenticationPolicy() override;
  const std::string &sourcePrincipal() override;
  const std::string &destinationPrincipal() override;
  const std::string &requestedServerName() override;

  // Direction
  bool isOutbound() override;

  // Header
  const std::string &referer() override;
  const std::string &userAgent() override;
  const std::string &requestID() override;
  const std::string &b3TraceID() override;
  const std::string &b3SpanID() override;
  bool b3TraceSampled() override;

private:
  ExtensionRootContext *getRootContext() {
    RootContext *root = this->root();
    return dynamic_cast<ExtensionRootContext *>(root);
  }

  ::istio::extension::StreamInfo stream_info_;
  bool use_traffic_data_ = true;
};

} // namespace Extension
} // namespace Istio
