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
#include "istio/extension/util/util.h"

namespace Istio {
namespace Extension {

using ServiceAuthenticationPolicy =
    ::Istio::Extension::StreamInfo::ServiceAuthenticationPolicy;

const std::set<std::string> kGrpcContentTypes{
    "application/grpc", "application/grpc+proto", "application/grpc+json"};

// Header keys
constexpr StringView kAuthorityHeaderKey = ":authority";
constexpr StringView kMethodHeaderKey = ":method";
constexpr StringView kContentTypeHeaderKey = "content-type";

const std::string kProtocolHTTP = "http";
const std::string kProtocolGRPC = "grpc";

constexpr StringView kMutualTLS = "MUTUAL_TLS";
constexpr StringView kNone = "NONE";

const char kBlackHoleCluster[] = "BlackHoleCluster";
const char kPassThroughCluster[] = "PassthroughCluster";
const char kInboundPassthroughClusterIpv4[] = "InboundPassthroughClusterIpv4";
const char kInboundPassthroughClusterIpv6[] = "InboundPassthroughClusterIpv6";
const char B3TraceID[] = "x-b3-traceid";
const char B3SpanID[] = "x-b3-spanid";
const char B3TraceSampled[] = "x-b3-sampled";

namespace {

// Extract service name from service host.
void extractServiceName(StringView host, StringView destination_namespace,
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
void getDestinationService(StringView dest_namespace, bool use_host_header,
                           std::string *dest_svc_host,
                           std::string *dest_svc_name) {
  std::string cluster_name;
  getValue({"cluster_name"}, &cluster_name);
  *dest_svc_host = use_host_header
                       ? getHeaderMapValue(HeaderMapType::RequestHeaders,
                                           kAuthorityHeaderKey)
                             ->toString()
                       : "unknown";

  if (cluster_name == kBlackHoleCluster ||
      cluster_name == kPassThroughCluster ||
      cluster_name == kInboundPassthroughClusterIpv4 ||
      cluster_name == kInboundPassthroughClusterIpv6) {
    *dest_svc_name = cluster_name;
    return;
  }

  std::vector<StringView> parts = absl::StrSplit(cluster_name, '|');
  if (parts.size() == 4) {
    *dest_svc_host = std::string(parts[3].data(), parts[3].size());
  }

  extractServiceName(*dest_svc_host, dest_namespace, dest_svc_name);
}

} // namespace

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
}

const istio::extension::NodeInfo &
ExtensionRootContext::getPeerNodeInfo(bool is_outbound) {
  return node_info_->getPeerNodeInfo(is_outbound);
}

const istio::extension::NodeInfo &ExtensionRootContext::getLocalNodeInfo() {
  return node_info_->getLocalNodeInfo();
}

const istio::extension::NodeInfo &ExtensionStreamContext::getSourceNodeInfo() {
  auto *root = getRootContext();
  bool is_outbound = isOutbound();
  return is_outbound ? root->getLocalNodeInfo()
                     : root->getPeerNodeInfo(/*is_outbound = */ false);
}

const istio::extension::NodeInfo &
ExtensionStreamContext::getDestinationNodeInfo() {
  auto *root = getRootContext();
  bool is_outbound = isOutbound();
  return is_outbound ? root->getPeerNodeInfo(/*is_outbound = */ true)
                     : root->getLocalNodeInfo();
}

bool ExtensionStreamContext::isOutbound() {
  if (!stream_info_.has_traffic_direction()) {
    int64_t direction;
    getValue({"listener_direction"}, &direction);
    stream_info_.mutable_traffic_direction()->set_value(direction);
  }
  return static_cast<TrafficDirection>(
             stream_info_.traffic_direction().value()) ==
         TrafficDirection::Outbound;
}

int64_t ExtensionStreamContext::requestTimestamp() {
  if (!stream_info_.has_request_timestamp()) {
    int64_t request_timestamp;
    getValue({"request", "time"}, &request_timestamp);
    stream_info_.mutable_request_timestamp()->set_value(request_timestamp);
  }
  return stream_info_.request_timestamp().value();
}

int64_t ExtensionStreamContext::responseTimestamp() {
  if (!stream_info_.has_response_timestamp()) {
    int64_t response_timestamp;
    getValue({"response", "time"}, &response_timestamp);
    stream_info_.mutable_response_timestamp()->set_value(response_timestamp);
  }
  return stream_info_.response_timestamp().value();
}

int64_t ExtensionStreamContext::duration() {
  if (!stream_info_.has_duration()) {
    int64_t duration;
    getValue({"request", "duration"}, &duration);
    stream_info_.mutable_duration()->set_value(duration);
  }
  return stream_info_.duration().value();
}

int64_t ExtensionStreamContext::responseDuration() {
  if (!stream_info_.has_response_duration()) {
    int64_t duration;
    getValue({"response", "duration"}, &duration);
    stream_info_.mutable_duration()->set_value(duration);
  }
  return stream_info_.response_duration().value();
}

const std::string &ExtensionStreamContext::responseFlag() {
  if (!stream_info_.has_response_flag()) {
    uint64_t response_flags = 0;
    getValue({"response", "flags"}, &response_flags);
    stream_info_.mutable_response_flag()->set_value(
        Util::parseResponseFlag(response_flags));
  }
  return stream_info_.response_flag().value();
}

int64_t ExtensionStreamContext::requestSize() {
  if (!stream_info_.has_request_size()) {
    int64_t request_total_size;
    getValue({"request", "total_size"}, &request_total_size);
    stream_info_.mutable_request_size()->set_value(request_total_size);
  }
  return stream_info_.request_size().value();
}

int64_t ExtensionStreamContext::responseSize() {
  if (!stream_info_.has_response_size()) {
    int64_t response_total_size;
    getValue({"response", "total_size"}, &response_total_size);
    stream_info_.mutable_response_size()->set_value(response_total_size);
  }
  return stream_info_.response_size().value();
}

int64_t ExtensionStreamContext::destinationPort() {
  if (!stream_info_.has_destination_port()) {
    int64_t destination_port = 0;
    if (isOutbound()) {
      getValue({"upstream", "port"}, &destination_port);
    } else {
      getValue({"destination", "port"}, &destination_port);
    }
    stream_info_.mutable_destination_port()->set_value(destination_port);
  }
  return stream_info_.destination_port().value();
}

const std::string &ExtensionStreamContext::sourceAddress() {
  if (!stream_info_.has_source_address()) {
    getValue({"source", "address"},
             stream_info_.mutable_source_address()->mutable_value());
  }
  return stream_info_.source_address().value();
}

const std::string &ExtensionStreamContext::destinationAddress() {
  if (!stream_info_.has_destination_address()) {
    getValue({"destination", "address"},
             stream_info_.mutable_destination_address()->mutable_value());
  }
  return stream_info_.destination_address().value();
}

const std::string &ExtensionStreamContext::requestProtocol() {
  if (!stream_info_.has_request_protocol()) {
    // TODO Add http/1.1, http/1.0, http/2 in a separate attribute.
    // http|grpc classification is compatible with Mixerclient
    if (kGrpcContentTypes.count(getHeaderMapValue(HeaderMapType::RequestHeaders,
                                                  kContentTypeHeaderKey)
                                    ->toString()) != 0) {
      stream_info_.mutable_request_protocol()->set_value(kProtocolGRPC);
    } else {
      stream_info_.mutable_request_protocol()->set_value(kProtocolHTTP);
    }
  }

  return stream_info_.request_protocol().value();
}

int64_t ExtensionStreamContext::responseCode() {
  if (!stream_info_.has_response_code()) {
    int64_t response_code;
    getValue({"response", "code"}, &response_code);
    stream_info_.mutable_response_code()->set_value(response_code);
  }
  return stream_info_.response_code().value();
}

const std::string &ExtensionStreamContext::destinationServiceHost() {
  if (!stream_info_.has_destination_service_host()) {
    // Get destination service name and host based on cluster name and host
    // header.
    const auto &destination_namespace = getDestinationNodeInfo().namespace_();
    getDestinationService(
        destination_namespace, use_traffic_data_,
        stream_info_.mutable_destination_service_host()->mutable_value(),
        stream_info_.mutable_destination_service_name()->mutable_value());
  }
  return stream_info_.destination_service_host().value();
}

const std::string &ExtensionStreamContext::destinationServiceName() {
  if (!stream_info_.has_destination_service_name()) {
    // Get destination service name and host based on cluster name and host
    // header.
    const auto &destination_namespace = getDestinationNodeInfo().namespace_();
    getDestinationService(
        destination_namespace, use_traffic_data_,
        stream_info_.mutable_destination_service_host()->mutable_value(),
        stream_info_.mutable_destination_service_name()->mutable_value());
  }
  return stream_info_.destination_service_name().value();
}

const std::string &ExtensionStreamContext::requestOperation() {
  if (!stream_info_.has_request_operation()) {
    getValue({"request", "method"},
             stream_info_.mutable_request_operation()->mutable_value());
  }
  return stream_info_.request_operation().value();
}

ServiceAuthenticationPolicy
ExtensionStreamContext::serviceAuthenticationPolicy() {
  if (isOutbound()) {
    return ServiceAuthenticationPolicy::Unspecified;
  }
  if (!stream_info_.has_mtls()) {
    bool mtls = false;
    getValue({"connection", "mtls"}, &mtls);
    stream_info_.mutable_mtls()->set_value(mtls);
  }
  return stream_info_.mtls().value() ? ServiceAuthenticationPolicy::MutualTLS
                                     : ServiceAuthenticationPolicy::None;
}

const std::string &ExtensionStreamContext::sourcePrincipal() {
  if (!stream_info_.has_source_principal()) {
    if (isOutbound()) {
      getValue({"upstream", "uri_san_local_certificate"},
               stream_info_.mutable_source_principal()->mutable_value());
    } else {
      getValue({"connection", "uri_san_peer_certificate"},
               stream_info_.mutable_source_principal()->mutable_value());
    }
  }
  return stream_info_.source_principal().value();
}

const std::string &ExtensionStreamContext::destinationPrincipal() {
  if (!stream_info_.has_destination_principal()) {
    if (isOutbound()) {
      getValue({"upstream", "uri_san_peer_certificate"},
               stream_info_.mutable_destination_principal()->mutable_value());
    } else {
      getValue({"connection", "uri_san_local_certificate"},
               stream_info_.mutable_destination_principal()->mutable_value());
    }
  }
  return stream_info_.destination_principal().value();
}

const std::string &ExtensionStreamContext::requestedServerName() {
  if (!stream_info_.has_requested_server_name()) {
    getValue({"connection", "requested_server_name"},
             stream_info_.mutable_requested_server_name()->mutable_value());
  }
  return stream_info_.requested_server_name().value();
}

const std::string &ExtensionStreamContext::referer() {
  if (!stream_info_.has_referer()) {
    getValue({"request", "referer"},
             stream_info_.mutable_referer()->mutable_value());
  }
  return stream_info_.referer().value();
}

const std::string &ExtensionStreamContext::userAgent() {
  if (!stream_info_.has_user_agent()) {
    getValue({"request", "user_agent"},
             stream_info_.mutable_user_agent()->mutable_value());
  }
  return stream_info_.user_agent().value();
}

const std::string &ExtensionStreamContext::urlPath() {
  if (!stream_info_.has_url_path()) {
    getValue({"request", "url_path"},
             stream_info_.mutable_url_path()->mutable_value());
  }
  return stream_info_.url_path().value();
}

const std::string &ExtensionStreamContext::requestHost() {
  if (!stream_info_.has_url_host()) {
    getValue({"request", "host"},
             stream_info_.mutable_url_host()->mutable_value());
  }
  return stream_info_.url_host().value();
}

const std::string &ExtensionStreamContext::requestScheme() {
  if (!stream_info_.has_url_scheme()) {
    getValue({"request", "scheme"},
             stream_info_.mutable_url_scheme()->mutable_value());
  }
  return stream_info_.url_scheme().value();
}

const std::string &ExtensionStreamContext::requestID() {
  if (!stream_info_.has_request_id()) {
    getValue({"request", "id"},
             stream_info_.mutable_request_id()->mutable_value());
  }
  return stream_info_.request_id().value();
}

const std::string &ExtensionStreamContext::b3SpanID() {
  if (!stream_info_.has_b3_span_id()) {
    getValue({"request", "headers", B3SpanID},
             stream_info_.mutable_b3_span_id()->mutable_value());
  }
  return stream_info_.b3_span_id().value();
}

const std::string &ExtensionStreamContext::b3TraceID() {
  if (!stream_info_.has_b3_span_id()) {
    getValue({"request", "headers", B3TraceID},
             stream_info_.mutable_b3_span_id()->mutable_value());
  }
  return stream_info_.b3_span_id().value();
}

bool ExtensionStreamContext::b3TraceSampled() {
  if (!stream_info_.has_b3_trace_sampled()) {
    bool sampled = false;
    getValue({"request", "headers", B3TraceSampled}, &sampled);
    stream_info_.mutable_b3_trace_sampled()->set_value(sampled);
  }
  return stream_info_.b3_trace_sampled().value();
}

} // namespace Extension
} // namespace Istio
