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

#include "istio/extension/node_info/node_info_cache.h"

namespace Istio {
namespace Extension {
namespace NodeInfo {

class NodeInfo {
public:
  NodeInfo();
  ~NodeInfo() {}

  // Get Local node metadata.
  const istio::extension::NodeInfo &getLocalNodeInfo();

  // Get node metadata of current active stream peer.
  const istio::extension::NodeInfo &getPeerNodeInfo(bool is_outbound);

private:
  // Local node info extracted from node metadata.
  istio::extension::NodeInfo local_node_info_;

  // Cache of peer node info.
  NodeInfoCache node_info_cache_;
};

} // namespace NodeInfo
} // namespace Extension
} // namespace Istio
