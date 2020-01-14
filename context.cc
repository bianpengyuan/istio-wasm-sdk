/* Copyright 2019 Istio Authors. All Rights Reserved.
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

#include "context.h"

#include "util.h"
#include "google/protobuf/util/json_util.h"

#include "proxy_wasm_intrinsics.h"

namespace Wasm {
namespace Common {

const char kRbacFilterName[] = "envoy.filters.http.rbac";
const char kRbacPermissivePolicyIDField[] = "shadow_effective_policy_id";
const char kRbacPermissiveEngineResultField[] = "shadow_engine_result";
const char kBlackHoleCluster[] = "BlackHoleCluster";
const char kPassThroughCluster[] = "PassthroughCluster";
const char kInboundPassthroughClusterIpv4[] = "InboundPassthroughClusterIpv4";
const char kInboundPassthroughClusterIpv6[] = "InboundPassthroughClusterIpv6";

namespace {

// Extract service name from service host.
void extractServiceName(const std::string& host,
                        const std::string& destination_namespace,
                        std::string* service_name) {
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
void getDestinationService(const std::string& dest_namespace,
                           bool use_host_header, std::string* dest_svc_host,
                           std::string* dest_svc_name) {
//   std::string cluster_name;
//   getValue({"cluster_name"}, &cluster_name);
//   *dest_svc_host = use_host_header
//                        ? getHeaderMapValue(HeaderMapType::RequestHeaders,
//                                            kAuthorityHeaderKey)
//                              ->toString()
//                        : "unknown";

//   if (cluster_name == kBlackHoleCluster ||
//       cluster_name == kPassThroughCluster ||
//       cluster_name == kInboundPassthroughClusterIpv4 ||
//       cluster_name == kInboundPassthroughClusterIpv6) {
//     *dest_svc_name = cluster_name;
//     return;
//   }

//   std::vector<std::string_view> parts = absl::StrSplit(cluster_name, '|');
//   if (parts.size() == 4) {
//     *dest_svc_host = std::string(parts[3].data(), parts[3].size());
//   }

//   extractServiceName(*dest_svc_host, dest_namespace, dest_svc_name);
}

void populateRequestInfo(bool outbound, bool use_host_header_fallback,
                         RequestInfo* request_info,
                         const std::string& destination_namespace) {
  // Fill in request info.
  // Get destination service name and host based on cluster name and host
  // header.
  getDestinationService(destination_namespace, use_host_header_fallback,
                        &request_info->destination_service_host,
                        &request_info->destination_service_name);

  // Get rbac labels from dynamic metadata.
  getValue({"metadata", kRbacFilterName, kRbacPermissivePolicyIDField},
           &request_info->rbac_permissive_policy_id);
  getValue({"metadata", kRbacFilterName, kRbacPermissiveEngineResultField},
           &request_info->rbac_permissive_engine_result);

  getValue({"request", "url_path"}, &request_info->request_url_path);

  if (outbound) {
    uint64_t destination_port = 0;
    getValue({"upstream", "port"}, &destination_port);
    request_info->destination_port = destination_port;
    getValue({"upstream", "uri_san_peer_certificate"},
             &request_info->destination_principal);
    getValue({"upstream", "uri_san_local_certificate"},
             &request_info->source_principal);
  } else {
    bool mtls = false;
    if (getValue({"connection", "mtls"}, &mtls)) {
      request_info->service_auth_policy =
          mtls ? ::Wasm::Common::ServiceAuthenticationPolicy::MutualTLS
               : ::Wasm::Common::ServiceAuthenticationPolicy::None;
    }
    getValue({"connection", "uri_san_local_certificate"},
             &request_info->destination_principal);
    getValue({"connection", "uri_san_peer_certificate"},
             &request_info->source_principal);
  }

  uint64_t response_flags = 0;
  getValue({"response", "flags"}, &response_flags);
  request_info->response_flag = parseResponseFlag(response_flags);

  getValue({"request", "time"}, &request_info->start_time);
  getValue({"request", "duration"}, &request_info->duration);
  getValue({"request", "total_size"}, &request_info->request_size);
  getValue({"response", "total_size"}, &request_info->response_size);
}

}  // namespace

StringView AuthenticationPolicyString(ServiceAuthenticationPolicy policy) {
  switch (policy) {
    case ServiceAuthenticationPolicy::None:
      return kNone;
    case ServiceAuthenticationPolicy::MutualTLS:
      return kMutualTLS;
    default:
      break;
  }
  return {};
  ;
}

// Retrieves the traffic direction from the configuration context.
TrafficDirection getTrafficDirection() {
  int64_t direction;
  if (getValue({"listener_direction"}, &direction)) {
    return static_cast<TrafficDirection>(direction);
  }
  return TrafficDirection::Unspecified;
}

using google::protobuf::util::JsonStringToMessage;
using google::protobuf::util::MessageToJsonString;

// Custom-written and lenient struct parser.
google::protobuf::util::Status extractNodeMetadata(
    const google::protobuf::Struct& metadata,
    wasm::common::NodeInfo* node_info) {
  for (const auto& it : metadata.fields()) {
    if (it.first == "NAME") {
      node_info->set_name(it.second.string_value());
    } else if (it.first == "NAMESPACE") {
      node_info->set_namespace_(it.second.string_value());
    } else if (it.first == "OWNER") {
      node_info->set_owner(it.second.string_value());
    } else if (it.first == "WORKLOAD_NAME") {
      node_info->set_workload_name(it.second.string_value());
    } else if (it.first == "ISTIO_VERSION") {
      node_info->set_istio_version(it.second.string_value());
    } else if (it.first == "MESH_ID") {
      node_info->set_mesh_id(it.second.string_value());
    } else if (it.first == "LABELS") {
      auto* labels = node_info->mutable_labels();
      for (const auto& labels_it : it.second.struct_value().fields()) {
        (*labels)[labels_it.first] = labels_it.second.string_value();
      }
    } else if (it.first == "PLATFORM_METADATA") {
      auto* platform_metadata = node_info->mutable_platform_metadata();
      for (const auto& platform_it : it.second.struct_value().fields()) {
        (*platform_metadata)[platform_it.first] =
            platform_it.second.string_value();
      }
    }
  }
  return google::protobuf::util::Status::OK;
}

google::protobuf::util::Status extractNodeMetadataGeneric(
    const google::protobuf::Struct& metadata,
    wasm::common::NodeInfo* node_info) {
  google::protobuf::util::JsonOptions json_options;
  std::string metadata_json_struct;
  auto status =
      MessageToJsonString(metadata, &metadata_json_struct, json_options);
  if (status != google::protobuf::util::Status::OK) {
    return status;
  }
  google::protobuf::util::JsonParseOptions json_parse_options;
  json_parse_options.ignore_unknown_fields = true;
  return JsonStringToMessage(metadata_json_struct, node_info,
                             json_parse_options);
}

google::protobuf::util::Status extractLocalNodeMetadata(
    wasm::common::NodeInfo* node_info) {
  google::protobuf::Struct node;
  if (!getMessageValue({"node", "metadata"}, &node)) {
    return google::protobuf::util::Status(
        google::protobuf::util::error::Code::NOT_FOUND, "metadata not found");
  }
  return extractNodeMetadata(node, node_info);
}

// Host header is used if use_host_header_fallback==true.
// Normally it is ok to use host header within the mesh, but not at ingress.
void populateHTTPRequestInfo(bool outbound, bool use_host_header_fallback,
                             RequestInfo* request_info,
                             const std::string& destination_namespace) {
  populateRequestInfo(outbound, use_host_header_fallback, request_info,
                      destination_namespace);

  int64_t response_code = 0;
  if (getValue({"response", "code"}, &response_code)) {
    request_info->response_code = response_code;
  }

  if (kGrpcContentTypes.count(getHeaderMapValue(HeaderMapType::RequestHeaders,
                                                kContentTypeHeaderKey)
                                  ->toString()) != 0) {
    request_info->request_protocol = kProtocolGRPC;
  } else {
    // TODO Add http/1.1, http/1.0, http/2 in a separate attribute.
    // http|grpc classification is compatible with Mixerclient
    request_info->request_protocol = kProtocolHTTP;
  }

  request_info->request_operation =
      getHeaderMapValue(HeaderMapType::RequestHeaders, kMethodHeaderKey)
          ->toString();

  if (!outbound) {
    uint64_t destination_port = 0;
    getValue({"destination", "port"}, &destination_port);
    request_info->destination_port = destination_port;
  }
}

void populateTCPRequestInfo(bool outbound, RequestInfo* request_info,
                            const std::string& destination_namespace) {
  // host_header_fallback is for HTTP/gRPC only.
  populateRequestInfo(outbound, false, request_info, destination_namespace);

  request_info->response_code = 0;
  request_info->request_protocol = kProtocolTCP;
}

google::protobuf::util::Status extractNodeMetadataValue(
    const google::protobuf::Struct& node_metadata,
    google::protobuf::Struct* metadata) {
  if (metadata == nullptr) {
    return google::protobuf::util::Status(
        google::protobuf::util::error::INVALID_ARGUMENT,
        "metadata provided is null");
  }
  const auto key_it = node_metadata.fields().find("EXCHANGE_KEYS");
  if (key_it == node_metadata.fields().end()) {
    return google::protobuf::util::Status(
        google::protobuf::util::error::INVALID_ARGUMENT,
        "metadata exchange key is missing");
  }

  const auto& keys_value = key_it->second;
  if (keys_value.kind_case() != google::protobuf::Value::kStringValue) {
    return google::protobuf::util::Status(
        google::protobuf::util::error::INVALID_ARGUMENT,
        "metadata exchange key is not a string");
  }

  // select keys from the metadata using the keys
  const std::set<std::string> keys;
  // const std::set<std::string> keys =
  //     absl::StrSplit(keys_value.string_value(), ',', absl::SkipWhitespace());
  for (auto key : keys) {
    const auto entry_it = node_metadata.fields().find(key);
    if (entry_it == node_metadata.fields().end()) {
      continue;
    }
    (*metadata->mutable_fields())[key] = entry_it->second;
  }

  return google::protobuf::util::Status(google::protobuf::util::error::OK, "");
}

}  // namespace Common
}  // namespace Wasm
