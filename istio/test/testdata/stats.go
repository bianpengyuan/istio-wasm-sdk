// Copyright 2020 Istio Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package testdata

var Stats = `stats_config:
  use_all_default_tags: true
  stats_tags:
  - tag_name: "reporter"
    regex: "(reporter=\\.=(.*?);\\.;)"
  - tag_name: "source_namespace"
    regex: "(source_namespace=\\.=(.*?);\\.;)"
  - tag_name: "source_workload"
    regex: "(source_workload=\\.=(.*?);\\.;)"
  - tag_name: "source_canonical_service"
    regex: "(source_canonical_service=\\.=(.*?);\\.;)"
  - tag_name: "source_canonical_revision"
    regex: "(source_canonical_revision=\\.=(.*?);\\.;)"
  - tag_name: "source_workload_namespace"
    regex: "(source_workload_namespace=\\.=(.*?);\\.;)"
  - tag_name: "source_principal"
    regex: "(source_principal=\\.=(.*?);\\.;)"
  - tag_name: "source_app"
    regex: "(source_app=\\.=(.*?);\\.;)"
  - tag_name: "source_version"
    regex: "(source_version=\\.=(.*?);\\.;)"
  - tag_name: "destination_namespace"
    regex: "(destination_namespace=\\.=(.*?);\\.;)"
  - tag_name: "destination_workload"
    regex: "(destination_workload=\\.=(.*?);\\.;)"
  - tag_name: "destination_workload_namespace"
    regex: "(destination_workload_namespace=\\.=(.*?);\\.;)"
  - tag_name: "destination_principal"
    regex: "(destination_principal=\\.=(.*?);\\.;)"
  - tag_name: "destination_app"
    regex: "(destination_app=\\.=(.*?);\\.;)"
  - tag_name: "destination_version"
    regex: "(destination_version=\\.=(.*?);\\.;)"
  - tag_name: "destination_service"
    regex: "(destination_service=\\.=(.*?);\\.;)"
  - tag_name: "destination_canonical_service"
    regex: "(destination_canonical_service=\\.=(.*?);\\.;)"
  - tag_name: "destination_canonical_revision"
    regex: "(destination_canonical_revision=\\.=(.*?);\\.;)"
  - tag_name: "destination_service_name"
    regex: "(destination_service_name=\\.=(.*?);\\.;)"
  - tag_name: "destination_service_namespace"
    regex: "(destination_service_namespace=\\.=(.*?);\\.;)"
  - tag_name: "request_protocol"
    regex: "(request_protocol=\\.=(.*?);\\.;)"
  - tag_name: "response_code"
    regex: "(response_code=\\.=(.*?);\\.;)|_rq(_(\\.d{3}))$"
  - tag_name: "grpc_response_status"
    regex: "(grpc_response_status=\\.=(.*?);\\.;)"
  - tag_name: "response_flags"
    regex: "(response_flags=\\.=(.*?);\\.;)"
  - tag_name: "connection_security_policy"
    regex: "(connection_security_policy=\\.=(.*?);\\.;)"
  # Extra regexes used for configurable metrics
  - tag_name: "configurable_metric_a"
    regex: "(configurable_metric_a=\\.=(.*?);\\.;)"
  - tag_name: "configurable_metric_b"
    regex: "(configurable_metric_b=\\.=(.*?);\\.;)"
  # Internal monitoring
  - tag_name: "cache"
    regex: "(cache\\.(.*?)\\.)"
  - tag_name: "component"
    regex: "(component\\.(.*?)\\.)"
  - tag_name: "tag"
    regex: "(tag\\.(.*?);\\.)"
  - tag_name: "wasm_filter"
    regex: "(wasm_filter\\.(.*?)\\.)"`
