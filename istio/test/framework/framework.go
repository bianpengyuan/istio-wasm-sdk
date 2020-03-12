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

package framework

import (
	"testing"

	"github.com/bianpengyuan/istio-wasm-sdk/istio/test/testdata"
)

var pa = PortAllocator{
	testIndex: 0,
}

type Test struct {
	gt *testing.T
	tec TestEnvoyConfig
	Ports *Ports
}

func NewTest(config TestEnvoyConfig, t *testing.T) *Test {
	// Set up default access log path if not set
	if config.ClientAccessLogPath == "" {
		config.ClientAccessLogPath = "/tmp/envoy-client-access.log"
	}
	if config.ServerAccessLogPath == "" {
		config.ServerAccessLogPath = "/tmp/envoy-server-access.log"
	}
	return &Test{
		gt: t,
		tec: config,
	}
}

func (t *Test) Run(fn func()) {
	// Alloc testing ports
	var err error
	t.tec.Ports, err = pa.AllocatePorts()
	if err != nil {
		t.gt.Errorf("cannot alloc valid port for the test: %v", err)
		return
	}

	// Set up default node metadata if not provided.
	if t.tec.ClientNodeMetadata == "" {
		t.tec.ClientNodeMetadata = testdata.ClientNodeMetadata
	}
	if t.tec.ServerNodeMetadata == "" {
		t.tec.ServerNodeMetadata = testdata.ServerNodeMetadata
	}

	ts := TestSetup{
		t:                   t.gt,
		tec:                 t.tec,
		startHTTPBackend:    true,
	}
	// Start up Envoyproxy
	if err = ts.SetUpClientServerEnvoy(); err != nil {
		t.gt.Errorf("cannot set up client side and server side envoy: %v", err)
		return
	}
	defer ts.TearDownClientServerEnvoy()
	// Start test
	fn()
}
