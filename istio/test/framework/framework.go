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
)

var pa = PortAllocator{
	testIndex: 0,
}

type Test struct {
	gt *testing.T
	tc TestConfig
	ports *Ports
}

func NewTest(config TestConfig, t *testing.T) *Test {
	// Set up default access log path if not set
	if config.ClientAccessLogPath == "" {
		config.ClientAccessLogPath = "/tmp/envoy-client-access.log"
	}
	if config.ServerAccessLogPath == "" {
		config.ServerAccessLogPath = "/tmp/envoy-server-access.log"
	}
	return &Test{
		gt: t,
		tc: config,
	}
}

func (t *Test) Run(fn func()) {
	// Alloc testing ports
	ports, err := pa.AllocatePorts()
	if err != nil {
		t.gt.Errorf("cannot alloc valid port for the test: %v", err)
		return
	}
	ts := TestSetup{
		t:                   t.gt,
		tc:                  t.tc,
		startHTTPBackend:    true,
		ports:               ports,
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
