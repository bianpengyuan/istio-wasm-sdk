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

#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

#include "istio/extension/node_info/node_info.pb.h"

namespace Istio {
namespace Extension {
namespace NodeInfo {

const size_t DefaultNodeCacheMaxSize = 500;

typedef std::shared_ptr<const istio::extension::NodeInfo> NodeInfoPtr;

class NodeInfoCache {
public:
  // Fetches and caches Peer information by peerId. An empty ptr will be
  // returned if any error conditions.
  // TODO Remove this when it is cheap to directly get it from StreamInfo.
  // At present this involves de-serializing to google.Protobuf.Struct and
  // then another round trip to NodeInfo. This Should at most hold N entries.
  // Node is owned by the cache. Do not store a reference.
  NodeInfoPtr getPeerById(const std::string &peer_metadata_id_key,
                          const std::string &peer_metadata_key);

  inline void setMaxCacheSize(int32_t size) {
    max_cache_size_ = size == 0 ? DefaultNodeCacheMaxSize : size;
  }

private:
  std::unordered_map<std::string, NodeInfoPtr> cache_;
  int32_t max_cache_size_ = DefaultNodeCacheMaxSize;
};

google::protobuf::util::Status
extractNodeMetadata(const google::protobuf::Struct &metadata,
                    istio::extension::NodeInfo *node_info);

} // namespace NodeInfo
} // namespace Extension
} // namespace Istio
