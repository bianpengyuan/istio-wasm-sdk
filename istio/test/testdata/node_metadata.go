// Copyright 2019 Istio Authors
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

var ClientNodeMetadata = `"CONFIG_NAMESPACE": "default",
"EXCHANGE_KEYS": "NAME,NAMESPACE,INSTANCE_IPS,LABELS,OWNER,PLATFORM_METADATA,WORKLOAD_NAME,CANONICAL_TELEMETRY_SERVICE,MESH_ID,SERVICE_ACCOUNT",
"INCLUDE_INBOUND_PORTS": "9080",
"INSTANCE_IPS": "10.52.0.34,fe80::a075:11ff:fe5e:f1cd",
"INTERCEPTION_MODE": "REDIRECT",
"ISTIO_PROXY_SHA": "istio-proxy:47e4559b8e4f0d516c0d17b233d127a3deb3d7ce",
"ISTIO_VERSION": "1.5-dev",
"LABELS": {
    "app": "productpage",
    "pod-template-hash": "84975bc778",
    "version": "v1",
    "service.istio.io/canonical-name": "productpage-v1",
    "service.istio.io/canonical-revision": "version-1"
},
"MESH_ID": "mesh",
"NAME": "productpage-v1-84975bc778-pxz2w",
"NAMESPACE": "default",
"OWNER": "kubernetes://apis/apps/v1/namespaces/default/deployments/productpage-v1",
"PLATFORM_METADATA": {
    "gcp_gke_cluster_name": "test-cluster",
    "gcp_location": "us-east4-b",
    "gcp_project": "test-project"
},
"POD_NAME": "productpage-v1-84975bc778-pxz2w",
"SERVICE_ACCOUNT": "bookinfo-productpage",
"STACKDRIVER_MONITORING_EXPORT_INTERVAL_SECS": "10",
"WORKLOAD_NAME": "productpage-v1",
"app": "productpage",
"istio": "sidecar",
"kubernetes.io/limit-ranger": "LimitRanger plugin set: cpu request for container productpage",
"pod-template-hash": "84975bc778",
"version": "v1"`

var ServerNodeMetadata = `"CONFIG_NAMESPACE": "default",
"EXCHANGE_KEYS": "NAME,NAMESPACE,INSTANCE_IPS,LABELS,OWNER,PLATFORM_METADATA,WORKLOAD_NAME,CANONICAL_TELEMETRY_SERVICE,MESH_ID,SERVICE_ACCOUNT",
"INCLUDE_INBOUND_PORTS": "9080",
"INSTANCE_IPS": "10.52.0.34,fe80::a075:11ff:fe5e:f1cd",
"INTERCEPTION_MODE": "REDIRECT",
"ISTIO_PROXY_SHA": "istio-proxy:47e4559b8e4f0d516c0d17b233d127a3deb3d7ce",
"ISTIO_VERSION": "1.5-dev",
"LABELS": {
    "app": "productpage",
    "pod-template-hash": "84975bc778",
    "version": "v1",
    "service.istio.io/canonical-name": "productpage-v1",
    "service.istio.io/canonical-revision": "version-1"
},
"MESH_ID": "mesh",
"NAME": "productpage-v1-84975bc778-pxz2w",
"NAMESPACE": "default",
"OWNER": "kubernetes://apis/apps/v1/namespaces/default/deployments/productpage-v1",
"PLATFORM_METADATA": {
    "gcp_gke_cluster_name": "test-cluster",
    "gcp_location": "us-east4-b",
    "gcp_project": "test-project"
},
"POD_NAME": "productpage-v1-84975bc778-pxz2w",
"SERVICE_ACCOUNT": "bookinfo-productpage",
"WORKLOAD_NAME": "productpage-v1",
"app": "productpage",
"istio": "sidecar",
"kubernetes.io/limit-ranger": "LimitRanger plugin set: cpu request for container productpage",
"pod-template-hash": "84975bc778",
"version": "v1"`
