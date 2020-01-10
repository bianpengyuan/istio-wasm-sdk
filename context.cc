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
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"

namespace Wasm {
namespace Common {

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
  const std::set<std::string> keys =
      absl::StrSplit(keys_value.string_value(), ',', absl::SkipWhitespace());
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
