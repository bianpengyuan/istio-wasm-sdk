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

#pragma once

#include <set>

#include "absl/strings/string_view.h"
#include "node_info.pb.h"
#include "google/protobuf/struct.pb.h"

namespace Wasm {
namespace Common {

using StringView = absl::string_view;

// Node metadata
const StringView WholeNodeKey = ".";

const StringView kUpstreamMetadataIdKey =
    "envoy.wasm.metadata_exchange.upstream_id";
const StringView kUpstreamMetadataKey =
    "envoy.wasm.metadata_exchange.upstream";

const StringView kDownstreamMetadataIdKey =
    "envoy.wasm.metadata_exchange.downstream_id";
const StringView kDownstreamMetadataKey =
    "envoy.wasm.metadata_exchange.downstream";

// Extracts NodeInfo from proxy node metadata passed in as a protobuf struct.
// It converts the metadata struct to a JSON struct and parse NodeInfo proto
// from that JSON struct.
// Returns status of protocol/JSON operations.
google::protobuf::util::Status extractNodeMetadata(
    const google::protobuf::Struct& metadata,
    wasm::common::NodeInfo* node_info);
google::protobuf::util::Status extractNodeMetadataGeneric(
    const google::protobuf::Struct& metadata,
    wasm::common::NodeInfo* node_info);

// Read from local node metadata and populate node_info.
google::protobuf::util::Status extractLocalNodeMetadata(
    wasm::common::NodeInfo* node_info);

// Extracts node metadata value. It looks for values of all the keys
// corresponding to EXCHANGE_KEYS in node_metadata and populates it in
// google::protobuf::Value pointer that is passed in.
google::protobuf::util::Status extractNodeMetadataValue(
    const google::protobuf::Struct& node_metadata,
    google::protobuf::Struct* metadata);

}  // namespace Common
}  // namespace Wasm
