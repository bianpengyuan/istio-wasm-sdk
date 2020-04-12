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
#include "istio/extension/util/util.h"

namespace Istio {
namespace Extension {

class ExtensionRootContext : public RootContext {
public:
  ExtensionRootContext(uint32_t id, StringView root_id)
      : RootContext(id, root_id) {
    node_info_ = std::make_unique<NodeInfo::NodeInfo>();
  }
  ~ExtensionRootContext() = default;

  // Gets peer node info. It checks the node info cache first, and then try to
  // fetch it from host if cache miss. If cache is disabled, it will fetch from
  // host directly.
  const istio::extension::NodeInfo &getPeerNodeInfo(bool is_outbound);

  // Get Local node information.
  const istio::extension::NodeInfo &getLocalNodeInfo();

private:
  std::unique_ptr<NodeInfo::NodeInfo> node_info_;
};

class ExtensionStreamContext : public Context {
public:
  ExtensionStreamContext(uint32_t id, RootContext *root) : Context(id, root){};
  ~ExtensionStreamContext() = default;

  /************************
        Node Property
  ************************/
  // TODO: labels and platform metadata
  const std::string &sourceName();
  const std::string &sourceNamespace();
  const std::string &sourceOwner();
  const std::string &sourceWorkloadName();
  const std::string &sourceIstioVersion();
  const std::string &sourceMeshID();

  const std::string &destinationName();
  const std::string &destinationNamespace();
  const std::string &destinationOwner();
  const std::string &destinationWorkloadName();
  const std::string &destinationIstioVersion();
  const std::string &destinationMeshID();

  /************************
      Request Property
  ************************/
  bool isOutbound();
  int64_t destinationPort();
  const std::string responseFlag();
  const std::string &requestProtocol();
  ServiceAuthenticationPolicy serviceAuthenticationPolicy();
  const std::string& sourcePrincipal();
  const std::string& destinationPrincipal();

  void destinationService(std::string *dest_host, std::string *dest_name);

private:
  ExtensionRootContext *getRootContext() {
    auto *root = this->root();
    return dynamic_cast<ExtensionRootContext *>(root);
  }

  const istio::extension::NodeInfo &sourceNodeInfo();
  const istio::extension::NodeInfo &destinationNodeInfo();

  std::string source_principal_;
  std::string destination_principal_;
};

} // namespace Extension
} // namespace Istio
