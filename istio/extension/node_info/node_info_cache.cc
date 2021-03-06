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

#include "google/protobuf/util/json_util.h"

using google::protobuf::util::Status;

namespace Istio {
namespace Extension {
namespace NodeInfo {

namespace {

// getNodeInfo fetches peer node info from host filter state. It returns true if
// no error occurs.
bool getNodeInfo(const std::string &peer_metadata_key,
                 istio::extension::NodeInfo *node_info) {
  google::protobuf::Struct metadata;
  std::string_view peer_metadata_key_view(peer_metadata_key.data(),
                                          peer_metadata_key.size());
  if (!getMessageValue({"filter_state", peer_metadata_key_view}, &metadata)) {
    LOG_DEBUG("cannot get metadata for: " + peer_metadata_key);
    return false;
  }

  auto status = extractNodeMetadata(metadata, node_info);
  if (status != Status::OK) {
    LOG_DEBUG("cannot parse peer node metadata " + metadata.DebugString() +
              ": " + status.ToString());
    return false;
  }
  return true;
}

} // namespace

// Custom-written and lenient struct parser.
google::protobuf::util::Status
extractNodeMetadata(const google::protobuf::Struct &metadata,
                    istio::extension::NodeInfo *node_info) {
  for (const auto &it : metadata.fields()) {
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
      auto *labels = node_info->mutable_labels();
      for (const auto &labels_it : it.second.struct_value().fields()) {
        (*labels)[labels_it.first] = labels_it.second.string_value();
      }
    } else if (it.first == "PLATFORM_METADATA") {
      auto *platform_metadata = node_info->mutable_platform_metadata();
      for (const auto &platform_it : it.second.struct_value().fields()) {
        (*platform_metadata)[platform_it.first] =
            platform_it.second.string_value();
      }
    }
  }
  return google::protobuf::util::Status::OK;
}

NodeInfoPtr NodeInfoCache::getPeerById(const std::string &peer_metadata_id_key,
                                       const std::string &peer_metadata_key) {
  if (max_cache_size_ < 0) {
    // Cache is disabled, fetch node info from host.
    auto node_info_ptr = std::make_shared<istio::extension::NodeInfo>();
    if (getNodeInfo(peer_metadata_key, node_info_ptr.get())) {
      return node_info_ptr;
    }
    return nullptr;
  }

  std::string peer_id;
  if (!getValue({"filter_state", peer_metadata_id_key}, &peer_id)) {
    LOG_DEBUG("cannot get metadata for: " + peer_metadata_id_key);
    return nullptr;
  }
  auto nodeinfo_it = cache_.find(peer_id);
  if (nodeinfo_it != cache_.end()) {
    return nodeinfo_it->second;
  }

  // Do not let the cache grow beyond max_cache_size_.
  if (int32_t(cache_.size()) > max_cache_size_) {
    auto it = cache_.begin();
    cache_.erase(cache_.begin(), std::next(it, max_cache_size_ / 4));
    LOG_INFO("cleaned cache, new cache_size:" + std::to_string(cache_.size()));
  }
  auto node_info_ptr = std::make_shared<istio::extension::NodeInfo>();
  if (getNodeInfo(peer_metadata_key, node_info_ptr.get())) {
    auto emplacement = cache_.emplace(peer_id, std::move(node_info_ptr));
    return emplacement.first->second;
  }
  return nullptr;
}

} // namespace NodeInfo
} // namespace Extension
} // namespace Istio
