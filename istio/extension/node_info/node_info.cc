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

#include "istio/extension/node_info/node_info.h"

#include "absl/strings/string_view.h"
#include "google/protobuf/stubs/status.h"
#include "istio/extension/util/util.h"
#include "proxy_wasm_intrinsics.h"

namespace Istio {
namespace Extension {
namespace NodeInfo {

const istio::extension::NodeInfo EmptyNodeInfo;

namespace {

google::protobuf::util::Status
extractLocalNodeMetadata(istio::extension::NodeInfo *node_info) {
  google::protobuf::Struct node;
  if (!getMessageValue({"node", "metadata"}, &node)) {
    return google::protobuf::util::Status(
        google::protobuf::util::error::Code::NOT_FOUND, "metadata not found");
  }
  return extractNodeMetadata(node, node_info);
}

} // namespace

// Node metadata
constexpr char WholeNodeKeyp[] = ".";
constexpr char UpstreamMetadataIdKey[] =
    "envoy.wasm.metadata_exchange.upstream_id";
constexpr char UpstreamMetadataKey[] = "envoy.wasm.metadata_exchange.upstream";
constexpr char DownstreamMetadataIdKey[] =
    "envoy.wasm.metadata_exchange.downstream_id";
constexpr char DownstreamMetadataKey[] =
    "envoy.wasm.metadata_exchange.downstream";

NodeInfo::NodeInfo() {
  auto status = extractLocalNodeMetadata(&local_node_info_);
  if (status != google::protobuf::util::Status::OK) {
    LOG_WARN("cannot extract local node metadata: " + status.ToString());
  }
}

const istio::extension::NodeInfo &NodeInfo::getLocalNodeInfo() {
  return local_node_info_;
}

const istio::extension::NodeInfo &NodeInfo::getPeerNodeInfo(bool is_outbound) {
  const auto &id_key =
      is_outbound ? UpstreamMetadataIdKey : DownstreamMetadataIdKey;
  const auto &metadata_key =
      is_outbound ? UpstreamMetadataKey : DownstreamMetadataKey;
  auto peer = node_info_cache_.getPeerById(id_key, metadata_key);
  if (!peer) {
    return EmptyNodeInfo;
  }
  return *peer;
}

} // namespace NodeInfo
} // namespace Extension
} // namespace Istio
