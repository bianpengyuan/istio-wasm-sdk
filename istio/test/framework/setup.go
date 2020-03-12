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
	"encoding/json"
	"fmt"
	"log"
	"net/http"

	"strings"
	"testing"
	"time"

	dto "github.com/prometheus/client_model/go"
	"github.com/prometheus/common/expfmt"
)

type TestEnvoyConfig struct {
	// EnvoyParams contain extra envoy parameters to pass in the CLI.
	EnvoyParams []string
	// ClientEnvoyTemplate is the bootstrap config used by client envoy.
	ClientEnvoyTemplate string
	// ServerEnvoyTemplate is the bootstrap config used by server envoy.
	ServerEnvoyTemplate string

	/* 
	*   Filters config
	*/
	// FiltersBeforeEnvoyRouterInAppToClient are the filters that come before envoy.router http filter in AppToClient
	// listener.
	FiltersBeforeEnvoyRouterInAppToClient string
	// FiltersBeforeHTTPConnectionManagerInProxyToServer are the filters that come before http connection manager filter
	// ProxyToServer listener.
	FiltersBeforeHTTPConnectionManagerInProxyToServer string
	// FiltersBeforeEnvoyRouterInProxyToServer are the filters that come before envoy.router http filter in
	// ProxyToServer listener.
	FiltersBeforeEnvoyRouterInProxyToServer string

	/* 
	*   Node metadata
	*/
	// Server side Envoy node metadata.
	ServerNodeMetadata string
	// Client side Envoy node metadata.
	ClientNodeMetadata string

	/* 
	*   Access log configuration
	*/
	// AccessLogPath is the access log path for Envoy
	AccessLogPath string
	// AccessLogPath is the access log path for the client Envoy
	ClientAccessLogPath string
	// AccessLogPath is the access log path for the server Envoy
	ServerAccessLogPath string
	// Format for client accesslog
	AccesslogFormat string
	// Format for server accesslog
	ServerAccesslogFormat string

	/* 
	*   TLS configuration
	*/
	// TLSContext to be used.
	TLSContext string
	// ClusterTLSContext to be used.
	ClusterTLSContext string
	// ServerTLSContext to be used.
	ServerTLSContext string
	// ServerClusterTLSContext to be used.
	ServerClusterTLSContext string
	// UpstreamFilters chain in client.
	UpstreamFiltersInClient string

	/* 
	*   Extra top level configuration
	*/
	// ExtraConfig that needs to be passed to envoy. Ex stats_config.
	ExtraConfig string

	// Allocated Ports for Envoy process.
	Ports       *Ports
}

// TestSetup store data for a test.
type TestSetup struct {
	tec TestEnvoyConfig

	// Dir is the working dir for envoy
	Dir string

	t           *testing.T
	clientEnvoy *Envoy
	serverEnvoy *Envoy
	httpBackend *HTTPServer
	tcpBackend  *TCPServer
	epoch       int
	// EnvoyConfigOpt allows passing additional parameters to the EnvoyTemplate
	EnvoyConfigOpt map[string]interface{}

	testName          uint16
	stress            bool
	noProxy           bool
	startHTTPBackend  bool
	disableHotRestart bool

	startTCPBackend   bool
	copyYamlFiles     bool
	EnableTLS         bool
}

// Stat represents a prometheus stat with labels.
type Stat struct {
	// Value of the metric
	Value int
	// Labels associated with the metric if any
	Labels map[string]string
}

// func NewClientServerEnvoyTestSetup(name uint16, t *testing.T) *TestSetup {
// 	return &TestSetup{
// 		t:                   t,
// 		startHTTPBackend:    true,
// 		Ports:               NewPorts(name),
// 		testName:            name,
// 		ClientAccessLogPath: "/tmp/envoy-client-access.log",
// 		ServerAccessLogPath: "/tmp/envoy-server-access.log",
// 	}
// }

// Ports get Ports object
// func (e *TestSetup) Ports() *Ports {
// 	return e.Ports
// }

// SetStress set the stress flag
func (s *TestSetup) SetStress(stress bool) {
	s.stress = stress
}

// SetDisableHotRestart sets whether disable the HotRestart feature of Envoy
func (s *TestSetup) SetDisableHotRestart(disable bool) {
	s.disableHotRestart = disable
}

// SetNoProxy set NoProxy flag
func (s *TestSetup) SetNoProxy(no bool) {
	s.noProxy = no
}

func (s *TestSetup) SetStartHTTPBackend(no bool) {
	s.startHTTPBackend = no
}

func (s *TestSetup) SetStartTCPBackend(yes bool) {
	s.startTCPBackend = yes
}

// SetCopyYamlFiles set copyYamlFiles flag
func (s *TestSetup) SetCopyYamlFiles(yes bool) {
	s.copyYamlFiles = yes
}

func (s *TestSetup) SetUpClientServerEnvoy() error {
	var err error

	log.Printf("Creating server envoy at %v", s.tec.Ports.ServerAdminPort)
	s.serverEnvoy, err = s.NewServerEnvoy()
	if err != nil {
		log.Printf("unable to create Envoy %v", err)
		return err
	}

	log.Printf("Starting server envoy at %v", s.tec.Ports.ServerAdminPort)
	err = s.serverEnvoy.Start(s.tec.Ports.ServerAdminPort)
	if err != nil {
		return err
	}

	log.Printf("Creating client envoy at %v", s.tec.Ports.ClientAdminPort)
	s.clientEnvoy, err = s.NewClientEnvoy()
	if err != nil {
		log.Printf("unable to create Envoy %v", err)
		return err
	}

	log.Printf("Starting client envoy at %v", s.tec.Ports.ClientAdminPort)
	err = s.clientEnvoy.Start(s.tec.Ports.ClientAdminPort)
	if err != nil {
		return err
	}

	if s.startHTTPBackend {
		s.httpBackend, err = NewHTTPServer(s.tec.Ports.BackendPort, s.EnableTLS, s.Dir)
		if err != nil {
			log.Printf("unable to create HTTP server %v", err)
		} else {
			errCh := s.httpBackend.Start()
			if err = <-errCh; err != nil {
				log.Fatalf("backend server start failed %v", err)
			}
		}
	}
	if s.startTCPBackend {
		s.tcpBackend, err = NewTCPServer(s.tec.Ports.BackendPort, "hello", s.EnableTLS, s.Dir)
		if err != nil {
			log.Printf("unable to create TCP server %v", err)
		} else {
			errCh := s.tcpBackend.Start()
			if err = <-errCh; err != nil {
				log.Fatalf("backend server start failed %v", err)
			}
		}
	}

	s.WaitClientEnvoyReady()
	s.WaitServerEnvoyReady()

	return nil
}

func (s *TestSetup) TearDownClientServerEnvoy() {
	if err := s.clientEnvoy.Stop(s.tec.Ports.ClientAdminPort); err != nil {
		s.t.Errorf("error quitting client envoy: %v", err)
	}
	s.clientEnvoy.TearDown()

	if err := s.serverEnvoy.Stop(s.tec.Ports.ServerAdminPort); err != nil {
		s.t.Errorf("error quitting client envoy: %v", err)
	}
	s.serverEnvoy.TearDown()

	if s.httpBackend != nil {
		s.httpBackend.Stop()
	}
	if s.tcpBackend != nil {
		s.tcpBackend.Stop()
	}
}

// LastRequestHeaders returns last backend request headers
func (s *TestSetup) LastRequestHeaders() http.Header {
	if s.httpBackend != nil {
		return s.httpBackend.LastRequestHeaders()
	}
	return nil
}

// WaitForStatsUpdateAndGetStats waits for waitDuration seconds to let Envoy update stats, and sends
// request to Envoy for stats. Returns stats response.
func (s *TestSetup) WaitForStatsUpdateAndGetStats(waitDuration int, port uint16) (string, error) {
	time.Sleep(time.Duration(waitDuration) * time.Second)
	statsURL := fmt.Sprintf("http://localhost:%d/stats?format=json&usedonly", port)
	code, respBody, err := HTTPGet(statsURL)
	if err != nil {
		return "", fmt.Errorf("sending stats request returns an error: %v", err)
	}
	if code != 200 {
		return "", fmt.Errorf("sending stats request returns unexpected status code: %d", code)
	}
	return respBody, nil
}

type statEntry struct {
	Name  string `json:"name"`
	Value int    `json:"value"`
}

type stats struct {
	StatList []statEntry `json:"stats"`
}

// WaitEnvoyReady waits until envoy receives and applies all config
func (s *TestSetup) WaitEnvoyReady(port uint16) {
	// Sometimes on circle CI, connection is refused even when envoy rePorts warm clusters and listeners...
	// Inject a 1 second delay to force readiness
	time.Sleep(1 * time.Second)

	delay := 200 * time.Millisecond
	total := 3 * time.Second
	var stats map[string]int
	for attempt := 0; attempt < int(total/delay); attempt++ {
		statsURL := fmt.Sprintf("http://localhost:%d/stats?format=json&usedonly", port)
		code, respBody, errGet := HTTPGet(statsURL)
		if errGet == nil && code == 200 {
			stats = s.unmarshalStats(respBody)
			warmingListeners, hasListeners := stats["listener_manager.total_listeners_warming"]
			warmingClusters, hasClusters := stats["cluster_manager.warming_clusters"]
			if hasListeners && hasClusters && warmingListeners == 0 && warmingClusters == 0 {
				return
			}
		}
		time.Sleep(delay)
	}

	s.t.Fatalf("envoy failed to get ready: %v", stats)
}

// WaitClientEnvoyReady waits until envoy receives and applies all config
func (s *TestSetup) WaitClientEnvoyReady() {
	s.WaitEnvoyReady(s.tec.Ports.ClientAdminPort)
}

// WaitEnvoyReady waits until envoy receives and applies all config
func (s *TestSetup) WaitServerEnvoyReady() {
	s.WaitEnvoyReady(s.tec.Ports.ServerAdminPort)
}

// UnmarshalStats Unmarshals Envoy stats from JSON format into a map, where stats name is
// key, and stats value is value.
func (s *TestSetup) unmarshalStats(statsJSON string) map[string]int {
	statsMap := make(map[string]int)

	var statsArray stats
	if err := json.Unmarshal([]byte(statsJSON), &statsArray); err != nil {
		s.t.Fatalf("unable to unmarshal stats from json")
	}

	for _, v := range statsArray.StatList {
		statsMap[v.Name] = v.Value
	}
	return statsMap
}

// VerifyEnvoyStats verifies Envoy stats.
func (s *TestSetup) VerifyEnvoyStats(expectedStats map[string]int, port uint16) {
	s.t.Helper()

	check := func(actualStatsMap map[string]int) error {
		for eStatsName, eStatsValue := range expectedStats {
			aStatsValue, ok := actualStatsMap[eStatsName]
			if !ok && eStatsValue != 0 {
				return fmt.Errorf("failed to find expected stat %s", eStatsName)
			}
			if aStatsValue != eStatsValue {
				return fmt.Errorf("stats %s does not match. expected vs actual: %d vs %d",
					eStatsName, eStatsValue, aStatsValue)
			}

			log.Printf("stat %s is matched. value is %d", eStatsName, eStatsValue)
		}
		return nil
	}

	delay := 200 * time.Millisecond
	total := 3 * time.Second

	var err error
	for attempt := 0; attempt < int(total/delay); attempt++ {
		statsURL := fmt.Sprintf("http://localhost:%d/stats?format=json&usedonly", port)
		code, respBody, errGet := HTTPGet(statsURL)
		if errGet != nil {
			log.Printf("sending stats request returns an error: %v", errGet)
		} else if code != 200 {
			log.Printf("sending stats request returns unexpected status code: %d", code)
		} else {
			actualStatsMap := s.unmarshalStats(respBody)
			for key, value := range actualStatsMap {
				log.Printf("key: %v, value %v", key, value)
			}
			if err = check(actualStatsMap); err == nil {
				return
			}
			log.Printf("failed to verify stats: %v", err)
		}
		time.Sleep(delay)
	}
	s.t.Errorf("failed to find expected stats: %v", err)
}

// VerifyPrometheusStats verifies prometheus stats.
func (s *TestSetup) VerifyPrometheusStats(expectedStats map[string]Stat, port uint16) {
	s.t.Helper()

	check := func(respBody string) error {
		var parser expfmt.TextParser
		reader := strings.NewReader(respBody)
		mapMetric, err := parser.TextToMetricFamilies(reader)
		if err != nil {
			return err
		}
		for eStatsName, eStatsValue := range expectedStats {
			aStats, ok := mapMetric[eStatsName]
			if !ok {
				return fmt.Errorf("failed to find expected stat %s", eStatsName)
			}
			var labels []*dto.LabelPair
			var aStatsValue float64
			switch aStats.GetType() {
			case dto.MetricType_COUNTER:
				if len(aStats.GetMetric()) != 1 {
					return fmt.Errorf("expected one value for counter")
				}
				aStatsValue = aStats.GetMetric()[0].GetCounter().GetValue()
				labels = aStats.GetMetric()[0].Label
			case dto.MetricType_GAUGE:
				if len(aStats.GetMetric()) != 1 {
					return fmt.Errorf("expected one value for gauge")
				}
				aStatsValue = aStats.GetMetric()[0].GetGauge().GetValue()
				labels = aStats.GetMetric()[0].Label
			default:
				return fmt.Errorf("need to implement this type %v", aStats.GetType())
			}
			if aStatsValue != float64(eStatsValue.Value) {
				return fmt.Errorf("stats %s does not match. expected vs actual: %v vs %v",
					eStatsName, eStatsValue, aStatsValue)
			}
			foundLabels := 0
			for _, label := range labels {
				v, found := eStatsValue.Labels[label.GetName()]
				if !found {
					continue
				}
				if v != label.GetValue() {
					return fmt.Errorf("metric %v label %v differs got:%v, want: %v", eStatsName, label.GetName(), label.GetValue(), v)
				}
				foundLabels++
			}
			if foundLabels != len(eStatsValue.Labels) {
				return fmt.Errorf("metrics %v, %d required labels missing", eStatsName, (len(eStatsValue.Labels) - foundLabels))
			}
		}
		return nil
	}

	delay := 200 * time.Millisecond
	total := 3 * time.Second

	var err error
	for attempt := 0; attempt < int(total/delay); attempt++ {
		statsURL := fmt.Sprintf("http://localhost:%d/stats/prometheus", port)
		code, respBody, errGet := HTTPGet(statsURL)
		if errGet != nil {
			log.Printf("sending stats request returns an error: %v", errGet)
		} else if code != 200 {
			log.Printf("sending stats request returns unexpected status code: %d", code)
		} else {
			if err = check(respBody); err == nil {
				return
			}
			log.Printf("failed to verify stats: %v", err)
		}
		time.Sleep(delay)
	}
	s.t.Errorf("failed to find expected stats: %v", err)
}

// VerifyStatsLT verifies that Envoy stats contains stat expectedStat, whose value is less than
// expectedStatVal.
func (s *TestSetup) VerifyStatsLT(actualStats string, expectedStat string, expectedStatVal int) {
	s.t.Helper()
	actualStatsMap := s.unmarshalStats(actualStats)

	aStatsValue, ok := actualStatsMap[expectedStat]
	if !ok {
		s.t.Fatalf("Failed to find expected Stat %s\n", expectedStat)
	} else if aStatsValue >= expectedStatVal {
		s.t.Fatalf("Stat %s does not match. Expected value < %d, actual stat value is %d",
			expectedStat, expectedStatVal, aStatsValue)
	} else {
		log.Printf("stat %s is matched. %d < %d", expectedStat, aStatsValue, expectedStatVal)
	}
}

func (s *TestSetup) StopHTTPBackend() {
	if s.httpBackend != nil {
		s.httpBackend.Stop()
	}
}
