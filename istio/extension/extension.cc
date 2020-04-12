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

#include "istio/extension/extension.h"

#include "absl/strings/str_split.h"

namespace Istio {
namespace Extension {

const std::set<std::string> kGrpcContentTypes{
    "application/grpc", "application/grpc+proto", "application/grpc+json"};

// Header keys
constexpr StringView AuthorityHeaderKey = ":authority";
constexpr StringView ContentTypeHeaderKey = "content-type";

const std::string kProtocolHTTP = "http";
const std::string kProtocolGRPC = "grpc";

const char kBlackHoleCluster[] = "BlackHoleCluster";
const char kPassThroughCluster[] = "PassthroughCluster";
const char kBlackHoleRouteName[] = "block_all";
const char kPassThroughRouteName[] = "allow_any";
const char kInboundPassthroughClusterIpv4[] = "InboundPassthroughClusterIpv4";
const char kInboundPassthroughClusterIpv6[] = "InboundPassthroughClusterIpv6";

namespace {

// Extract service name from service host.
void extractServiceName(const std::string &host,
                        const std::string &destination_namespace,
                        std::string *service_name) {
  auto name_pos = host.find_first_of(".:");
  if (name_pos == std::string::npos) {
    // host is already a short service name. return it directly.
    *service_name = host;
    return;
  }
  if (host[name_pos] == ':') {
    // host is `short_service:port`, return short_service name.
    *service_name = host.substr(0, name_pos);
    return;
  }

  auto namespace_pos = host.find_first_of(".:", name_pos + 1);
  std::string service_namespace = "";
  if (namespace_pos == std::string::npos) {
    service_namespace = host.substr(name_pos + 1);
  } else {
    int namespace_size = namespace_pos - name_pos - 1;
    service_namespace = host.substr(name_pos + 1, namespace_size);
  }
  // check if namespace in host is same as destination namespace.
  // If it is the same, return the first part of host as service name.
  // Otherwise fallback to request host.
  if (service_namespace == destination_namespace) {
    *service_name = host.substr(0, name_pos);
  } else {
    *service_name = host;
  }
}

// Get destination service host and name based on destination cluster name and
// host header.
// * If cluster name is one of passthrough and blackhole clusters, use cluster
//   name as destination service name and host header as destination host.
// * If cluster name follows Istio convention (four parts separated by pipe),
//   use the last part as destination host; Otherwise, use host header as
//   destination host. To get destination service name from host: if destination
//   host is already a short name, use that as destination service; otherwise if
//   the second part of destination host is destination namespace, use first
//   part as destination service name. Otherwise, fallback to use destination
//   host for destination service name.
void getDestinationService(const std::string &dest_namespace,
                           std::string *dest_svc_host,
                           std::string *dest_svc_name) {
  std::string cluster_name;
  getValue({"cluster_name"}, &cluster_name);
  *dest_svc_host =
      getHeaderMapValue(HeaderMapType::RequestHeaders, AuthorityHeaderKey)
          ->toString();

  // override the cluster name if this is being sent to the
  // blackhole or passthrough cluster
  std::string route_name;
  getValue({"route_name"}, &route_name);
  if (route_name == kBlackHoleRouteName) {
    cluster_name = kBlackHoleCluster;
  } else if (route_name == kPassThroughRouteName) {
    cluster_name = kPassThroughCluster;
  }

  if (cluster_name == kBlackHoleCluster ||
      cluster_name == kPassThroughCluster ||
      cluster_name == kInboundPassthroughClusterIpv4 ||
      cluster_name == kInboundPassthroughClusterIpv6) {
    *dest_svc_name = cluster_name;
    return;
  }

  std::vector<absl::string_view> parts = absl::StrSplit(cluster_name, '|');
  if (parts.size() == 4) {
    *dest_svc_host = std::string(parts[3].data(), parts[3].size());
  }

  extractServiceName(*dest_svc_host, dest_namespace, dest_svc_name);
}

} // namespace

const istio::extension::NodeInfo &
ExtensionRootContext::getPeerNodeInfo(bool is_outbound) {
  return node_info_->getPeerNodeInfo(is_outbound);
}

const istio::extension::NodeInfo &ExtensionRootContext::getLocalNodeInfo() {
  return node_info_->getLocalNodeInfo();
}

/************************
    Node Property
************************/

#define NODE_ATTRIBUTE_FUNC(_p, an, _fn)                                       \
  const std::string &ExtensionStreamContext::_p##_fn() {                       \
    auto &node = _p##NodeInfo();                                               \
    return node.an();                                                          \
  }

NODE_ATTRIBUTE_FUNC(source, name, Name)
NODE_ATTRIBUTE_FUNC(source, namespace_, Namespace)
NODE_ATTRIBUTE_FUNC(source, namespace_, Owner)
NODE_ATTRIBUTE_FUNC(source, workload_name, WorkloadName)
NODE_ATTRIBUTE_FUNC(source, istio_version, IstioVersion)
NODE_ATTRIBUTE_FUNC(source, mesh_id, MeshID)

NODE_ATTRIBUTE_FUNC(destination, name, Name)
NODE_ATTRIBUTE_FUNC(destination, namespace_, Namespace)
NODE_ATTRIBUTE_FUNC(destination, namespace_, Owner)
NODE_ATTRIBUTE_FUNC(destination, workload_name, WorkloadName)
NODE_ATTRIBUTE_FUNC(destination, istio_version, IstioVersion)
NODE_ATTRIBUTE_FUNC(destination, mesh_id, MeshID)

/************************
    Request Property
************************/

// Direction
bool ExtensionStreamContext::isOutbound() {
  int64_t direction;
  getValue({"listener_direction"}, &direction);
  return static_cast<TrafficDirection>(direction) == TrafficDirection::Outbound;
}

// Connection
int64_t ExtensionStreamContext::destinationPort() {
  int64_t destination_port = 0;
  if (isOutbound()) {
    getValue({"upstream", "port"}, &destination_port);
  } else {
    getValue({"destination", "port"}, &destination_port);
  }
  return destination_port;
}

// Response flag
const std::string ExtensionStreamContext::responseFlag() {
  uint64_t response_flags_mask = 0;
  getValue({"response", "flags"}, &response_flags_mask);
  return Util::parseResponseFlag(response_flags_mask);
}

const std::string &ExtensionStreamContext::requestProtocol() {
  if (kGrpcContentTypes.count(
          getHeaderMapValue(HeaderMapType::RequestHeaders, ContentTypeHeaderKey)
              ->toString()) != 0) {
    return kProtocolGRPC;
  }
  return kProtocolHTTP;
}

void ExtensionStreamContext::destinationService(std::string *dest_host,
                                                std::string *dest_name) {
  const auto &destination_namespace = destinationNodeInfo().namespace_();
  getDestinationService(destination_namespace, dest_host, dest_name);
}

ServiceAuthenticationPolicy
ExtensionStreamContext::serviceAuthenticationPolicy() {
  if (isOutbound()) {
    return ServiceAuthenticationPolicy::Unspecified;
  }
  bool mtls = false;
  getValue({"connection", "mtls"}, &mtls);
  return mtls ? ServiceAuthenticationPolicy::MutualTLS
              : ServiceAuthenticationPolicy::None;
}

const std::string& ExtensionStreamContext::sourcePrincipal() {
  if (isOutbound()) {
    getValue({"upstream", "uri_san_local_certificate"}, &source_principal_);
  } else {
    getValue({"connection", "uri_san_peer_certificate"}, &source_principal_);
  }
  return source_principal_;
}

const std::string& ExtensionStreamContext::destinationPrincipal() {
  if (isOutbound()) {
    getValue({"upstream", "uri_san_peer_certificate"}, &destination_principal_);
  } else {
    getValue({"connection", "uri_san_local_certificate"}, &destination_principal_);
  }
  return destination_principal_;
}

const istio::extension::NodeInfo &ExtensionStreamContext::sourceNodeInfo() {
  auto *root = getRootContext();
  bool is_outbound = isOutbound();
  return is_outbound ? root->getLocalNodeInfo()
                     : root->getPeerNodeInfo(/*is_outbound = */ false);
}

const istio::extension::NodeInfo &
ExtensionStreamContext::destinationNodeInfo() {
  auto *root = getRootContext();
  bool is_outbound = isOutbound();
  return is_outbound ? root->getPeerNodeInfo(/*is_outbound = */ true)
                     : root->getLocalNodeInfo();
}

} // namespace Extension
} // namespace Istio
