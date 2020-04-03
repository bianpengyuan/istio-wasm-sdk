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

var ClientBootstrap = `node:
  id: client
  cluster: test-cluster
  metadata: { {{ .Vars.ClientMetadata }} }
admin:
  access_log_path: /dev/null
  address:
    socket_address:
      address: 127.0.0.1
      port_value: {{ .Ports.ClientAdminPort }}
{{ .Vars.StatsConfig }}
dynamic_resources:
  ads_config:
    api_type: GRPC
    grpc_services:
    - envoy_grpc:
        cluster_name: xds_cluster
  cds_config:
    ads: {}
  lds_config:
    ads: {}
static_resources:
  clusters:
  - connect_timeout: 1s
    load_assignment:
      cluster_name: xds_cluster
      endpoints:
      - lb_endpoints:
        - endpoint:
            address:
              socket_address:
                address: 127.0.0.1
                port_value: {{ .Ports.XDSPort }}
    http2_protocol_options: {}
    name: xds_cluster
  - name: outbound|9080|http|server.default.svc.cluster.local
    connect_timeout: 1s
    type: STATIC
    http2_protocol_options: {}
    load_assignment:
      cluster_name: outbound|9080|http|server.default.svc.cluster.local
      endpoints:
      - lb_endpoints:
        - endpoint:
            address:
              socket_address:
                address: 127.0.0.1
                port_value: {{ .Ports.ServerPort }}
{{ .Vars.ClientTLSContext | indent 4 }}
{{ .Vars.ClientStaticCluster | indent 2 }}`

var ServerBootstrap = `node:
  id: server
  cluster: test-cluster
  metadata: { {{ .Vars.ServerMetadata }} }
admin:
  access_log_path: /dev/null
  address:
    socket_address:
      address: 127.0.0.1
      port_value: {{ .Ports.ServerAdminPort }}
{{ .Vars.StatsConfig }}
dynamic_resources:
  ads_config:
    api_type: GRPC
    grpc_services:
    - envoy_grpc:
        cluster_name: xds_cluster
  cds_config:
    ads: {}
  lds_config:
    ads: {}
static_resources:
  clusters:
  - connect_timeout: 1s
    load_assignment:
      cluster_name: xds_cluster
      endpoints:
      - lb_endpoints:
        - endpoint:
            address:
              socket_address:
                address: 127.0.0.1
                port_value: {{ .Ports.XDSPort }}
    http2_protocol_options: {}
    name: xds_cluster
  - name: inbound|9080|http|server.default.svc.cluster.local
    connect_timeout: 1s
    type: STATIC
    load_assignment:
      cluster_name: inbound|9080|http|server.default.svc.cluster.local
      endpoints:
      - lb_endpoints:
        - endpoint:
            address:
              socket_address:
                address: 127.0.0.1
                port_value: {{ .Ports.BackendPort }}
{{ .Vars.ServerStaticCluster | indent 2 }}`
