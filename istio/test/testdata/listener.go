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

// DefaultClientListener ...
var DefaultClientListener = `{{- if ne .Vars.ClientListeners "" }}
{{ .Vars.ClientListeners }}
{{- else }}
name: client
traffic_direction: OUTBOUND
address:
  socket_address:
    address: 127.0.0.1
    port_value: {{ .Ports.ClientPort }}
filter_chains:
- filters:
  - name: envoy.http_connection_manager
    typed_config:
      "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
      codec_type: AUTO
      stat_prefix: client
      http_filters:
      - name: envoy.filters.http.wasm
        typed_config:
          "@type": type.googleapis.com/udpa.type.v1.TypedStruct
          type_url: envoy.extensions.filters.http.wasm.v3.Wasm
          value:
            config:
              vm_config:
                runtime: envoy.wasm.runtime.null
                code:
                  local:
                    inline_string: envoy.wasm.metadata_exchange
              configuration: "{ max_peer_cache_size: 20 }"
{{- if ne .Vars.ClientHTTPFilters "" }}
{{ .Vars.ClientHTTPFilters | indent 6 }}
{{- end }}
      - name: envoy.router
      route_config:
        name: client
        virtual_hosts:
        - name: client
          domains: ["*"]
          routes:
          - match: { prefix: / }
            route:
              cluster: outbound|9080|http|server.default.svc.cluster.local
              timeout: 0s
{{- end }}`

// DefaultServerListener ...
var DefaultServerListener = `{{- if ne .Vars.ServerListeners "" }}
{{ .Vars.ServerListeners }}
{{- else }}
name: server
traffic_direction: INBOUND
address:
  socket_address:
    address: 127.0.0.1
    port_value: {{ .Ports.ServerPort }}
filter_chains:
- filters:
  - name: envoy.http_connection_manager
    typed_config:
      "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
      codec_type: AUTO
      stat_prefix: server
      http_filters:
      - name: envoy.filters.http.wasm
        typed_config:
          "@type": type.googleapis.com/udpa.type.v1.TypedStruct
          type_url: envoy.extensions.filters.http.wasm.v3.Wasm
          value:
            config:
              vm_config:
                runtime: envoy.wasm.runtime.null
                code:
                  local:
                    inline_string: envoy.wasm.metadata_exchange
              configuration: "{ max_peer_cache_size: 20 }"
{{- if ne .Vars.ServerHTTPFilters "" }}
{{ .Vars.ServerHTTPFilters | indent 6 }}
{{- end }}
      - name: envoy.router
      route_config:
        name: server
        virtual_hosts:
        - name: server
          domains: ["*"]
          routes:
          - match: { prefix: / }
            route:
              cluster: inbound|9080|http|server.default.svc.cluster.local
              timeout: 0s
{{ .Vars.ServerTLSContext | indent 2 }}
{{- end }}`
