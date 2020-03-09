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

package framework

import (
	"errors"
	"log"
	"sync"
)

// Dynamic port allocation scheme
// In order to run the tests in parallel. Each test should use unique ports
// Each test has a unique test_name, its ports will be allocated based on that name

const (
	portBase uint16 = 20000
	// Maximum number of ports used in each test.
	portNum uint16 = 20
)

type PortAllocator struct {
	testIndex uint16
	mux sync.Mutex
}

// Ports stores all used ports
type Ports struct {
	BackendPort             uint16
	ClientAdminPort         uint16
	AppToClientProxyPort    uint16
	ClientToServerProxyPort uint16
	ServerAdminPort         uint16
	XDSPort                 uint16
	SDPort                  uint16
}

func (p *PortAllocator) AllocatePorts() (*Ports, error) {
	p.mux.Lock()
	p.mux.Unlock()
	base := portBase + p.testIndex*portNum
	for base+portNum <= 65535 {
		if allPortFree(base, portNum) {
			break
		}
		base += p.testIndex * portNum
	}
	if base + portNum > 65535 {
		return nil, errors.New("Cannot find valid port range for test")
	}
	p.testIndex += 1
	return &Ports{
		BackendPort:             base,
		ClientAdminPort:         base + 1,
		AppToClientProxyPort:    base + 2,
		ClientToServerProxyPort: base + 3,
		ServerAdminPort:         base + 4,
		XDSPort:                 base + 5,
		SDPort:                  base + 6,
	}, nil
}

func allPortFree(base uint16, ports uint16) bool {
	for port := base; port < base+ports; port++ {
		if IsPortUsed(port) {
			log.Println("port is used ", port)
			return false
		}
	}
	return true
}

// NewPorts allocate all ports based on test id.
// func NewPorts(name uint16) *Ports {
// 	base := allocPortBase(name)
// 	return &Ports{
// 		BackendPort:             base,
// 		ClientAdminPort:         base + 1,
// 		AppToClientProxyPort:    base + 2,
// 		ClientToServerProxyPort: base + 3,
// 		ServerAdminPort:         base + 4,
// 		XDSPort:                 base + 5,
// 		SDPort:                  base + 6,
// 	}
// }
