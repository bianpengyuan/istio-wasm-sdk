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
	"encoding/json"
	"fmt"
	"go/build"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"time"

	core "github.com/envoyproxy/go-control-plane/envoy/api/v2/core"
	v2 "github.com/envoyproxy/go-control-plane/envoy/config/bootstrap/v2"

	// Preload proto definitions
	_ "github.com/cncf/udpa/go/udpa/type/v1"
	_ "github.com/envoyproxy/go-control-plane/envoy/extensions/filters/network/http_connection_manager/v3"

	"github.com/bianpengyuan/istio-wasm-sdk/istio/test/testdata"
)

type Envoy struct {
	// template for the bootstrap
	Bootstrap string

	tmpFile   string
	cmd       *exec.Cmd
	adminPort uint32

	branch string
}

var _ Step = &Envoy{}

func (e *Envoy) Run(p *Params) error {
	// Fill in default Vars if not provided
	fillDefaultBootstrapVars(p)
	bootstrap, err := p.Fill(e.Bootstrap)
	if err != nil {
		return err
	}
	log.Printf("envoy bootstrap:\n%s\n", bootstrap)

	e.adminPort, err = getAdminPort(bootstrap)
	if err != nil {
		return err
	}
	log.Printf("admin port %d", e.adminPort)

	tmp, err := ioutil.TempFile(os.TempDir(), "envoy-bootstrap-*.yaml")
	if err != nil {
		return err
	}
	if _, err := tmp.Write([]byte(bootstrap)); err != nil {
		return err
	}
	e.tmpFile = tmp.Name()

	debugLevel, ok := os.LookupEnv("ENVOY_DEBUG")
	if !ok {
		debugLevel = "info"
	}
	args := []string{
		"-c", e.tmpFile,
		"-l", debugLevel,
		"--concurrency", "1",
		"--disable-hot-restart",
		"--drain-time-s", "4", // this affects how long draining listenrs are kept alive
	}
	e.branch = "master"
	if version, exists := os.LookupEnv("ENVOY_VERSION"); exists {
		e.branch = version
	}
	envoyPath := ""
	if path, exists := os.LookupEnv("ENVOY_PATH"); exists {
		envoyPath = path
	} else if envoyPath, err = downloadEnvoy(e.branch); err != nil {
		return err
	}
	cmd := exec.Command(envoyPath, args...)
	cmd.Stderr = os.Stderr
	cmd.Stdout = os.Stdout

	log.Printf("envoy cmd %v", cmd.Args)
	e.cmd = cmd
	if err = cmd.Start(); err != nil {
		return err
	}

	url := fmt.Sprintf("http://127.0.0.1:%v/ready", e.adminPort)
	return WaitForHTTPServer(url)
}

func (e *Envoy) Cleanup() {
	log.Printf("stop envoy ...\n")
	if e.cmd != nil {
		url := fmt.Sprintf("http://127.0.0.1:%v/quitquitquit", e.adminPort)
		_, _, _, _ = HTTPPost(url, "", map[string][]string{}, "")
		done := make(chan error, 1)
		go func() {
			done <- e.cmd.Wait()
		}()
		select {
		case <-time.After(3 * time.Second):
			log.Println("envoy killed as timeout reached")
			log.Println(e.cmd.Process.Kill())
		case err := <-done:
			log.Printf("stop envoy ... done\n")
			if err != nil {
				log.Println(err)
			}
		}
	}

	log.Println("removing temp config file")
	os.Remove(e.tmpFile)
}

func getAdminPort(bootstrap string) (uint32, error) {
	pb := &v2.Bootstrap{}
	if err := ReadYAML(bootstrap, pb); err != nil {
		return 0, err
	}
	if pb.Admin == nil || pb.Admin.Address == nil {
		return 0, fmt.Errorf("missing admin section in bootstrap: %v", bootstrap)
	}
	socket, ok := pb.Admin.Address.Address.(*core.Address_SocketAddress)
	if !ok {
		return 0, fmt.Errorf("missing socket in bootstrap: %v", bootstrap)
	}
	port, ok := socket.SocketAddress.PortSpecifier.(*core.SocketAddress_PortValue)
	if !ok {
		return 0, fmt.Errorf("missing port in bootstrap: %v", bootstrap)
	}
	return port.PortValue, nil
}

func fillDefaultBootstrapVars(p *Params) {
	if _, ok := p.Vars["ClientMetadata"]; !ok {
		p.Vars["ClientMetadata"] = testdata.ClientNodeMetadata
	}
	if _, ok := p.Vars["ServerMetadata"]; !ok {
		p.Vars["ServerMetadata"] = testdata.ServerNodeMetadata
	}
	if _, ok := p.Vars["StatsConfig"]; !ok {
		p.Vars["StatsConfig"] = testdata.Stats
	}
}

// downloads env based on the given branch name. Return address of donwloaded envoy.
func downloadEnvoy(ver string) (string, error) {
	proxyDepUrl := fmt.Sprintf("https://raw.githubusercontent.com/istio/istio/%v/istio.deps", ver)
	resp, err := http.Get(proxyDepUrl)
	if err != nil {
		return "", fmt.Errorf("cannot get envoy sha from %v: %v", proxyDepUrl, err)
	}
	defer resp.Body.Close()
	istioDeps, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return "", fmt.Errorf("cannot read body of istio deps: %v", err)
	}

	var deps []interface{}
	json.Unmarshal([]byte(istioDeps), &deps)
	proxySHA := ""
	for _, d := range deps {
		if dm, ok := d.(map[string]interface{}); ok && dm["repoName"].(string) == "proxy" {
			proxySHA = dm["lastStableSHA"].(string)
		}
	}

	dst := fmt.Sprintf("%v/envoy-%v", getDefaultPluginEnvoyOutDir(), proxySHA)
	if _, err := os.Stat(dst); err == nil {
		return dst, nil
	}
	envoyURL := fmt.Sprintf("https://storage.googleapis.com/istio-build/proxy/envoy-alpha-%v.tar.gz", proxySHA)
	donwloadCmd := exec.Command("bash", "-c", fmt.Sprintf("curl -fLSs %v | tar xz", envoyURL))
	donwloadCmd.Stderr = os.Stderr
	donwloadCmd.Stdout = os.Stdout
	err = donwloadCmd.Run()
	if err != nil {
		return "", fmt.Errorf("fail to run envoy download command: %v", err)
	}
	src := "usr/local/bin/envoy"
	if _, err := os.Stat(src); err != nil {
		return "", fmt.Errorf("fail to find downloaded envoy: %v", err)
	}
	defer os.RemoveAll("usr/")

	cpCmd := exec.Command("cp", src, dst)
	cpCmd.Stderr = os.Stderr
	cpCmd.Stdout = os.Stdout
	if err := cpCmd.Run(); err != nil {
		return "", fmt.Errorf("fail to copy envoy binary from %v to %v: %v", src, dst, err)
	}

	return dst, nil
}

func getDefaultPluginEnvoyOutDir() string {
	dir := fmt.Sprintf("%s/out/wasm_plugin_test", build.Default.GOPATH)
	_ = os.Mkdir(dir, 0700)
	return dir
}

// ClientEnvoy models a default client side proxy
type ClientEnvoy struct {
	e *Envoy
}

var _ Step = &ClientEnvoy{}

func (c *ClientEnvoy) Run(p *Params) error {
	u := Update{
		Node:      "client",
		Version:   "0",
		Listeners: []string{testdata.DefaultClientListener},
	}
	if err := u.Run(p); err != nil {
		return err
	}
	c.e = &Envoy{
		Bootstrap: testdata.ClientBootstrap,
	}
	if err := c.e.Run(p); err != nil {
		return err
	}
	return nil
}

func (c *ClientEnvoy) Cleanup() {
	c.e.Cleanup()
}

// ServerEnvoy models a default server side proxy
type ServerEnvoy struct {
	e           *Envoy
	httpBackend *HTTPServer
}

var _ Step = &ServerEnvoy{}

func (s *ServerEnvoy) Run(p *Params) error {
	u := Update{
		Node:      "server",
		Version:   "0",
		Listeners: []string{testdata.DefaultServerListener},
	}
	if err := u.Run(p); err != nil {
		return err
	}
	s.e = &Envoy{
		Bootstrap: testdata.ServerBootstrap,
	}
	var err error
	if err = s.e.Run(p); err != nil {
		return err
	}

	s.httpBackend, err = NewHTTPServer(p.Ports.BackendPort, false, "")
	if err != nil {
		return fmt.Errorf("unable to create HTTP server %v", err)
	}

	errCh := s.httpBackend.Start()
	if err = <-errCh; err != nil {
		return fmt.Errorf("backend server start failed %v", err)
	}
	return nil
}

func (s *ServerEnvoy) Cleanup() {
	s.e.Cleanup()
	s.httpBackend.Stop()
}

// ClientServerEnvoy models a default client side and server side proxy
type ClientServerEnvoy struct {
	se          *ServerEnvoy
	ce          *ClientEnvoy
	httpBackend *HTTPServer
}

var _ Step = &ServerEnvoy{}

func (cs *ClientServerEnvoy) Run(p *Params) error {
	cs.ce = &ClientEnvoy{
		e: &Envoy{Bootstrap: testdata.ClientBootstrap},
	}
	if err := cs.ce.Run(p); err != nil {
		return err
	}

	cs.se = &ServerEnvoy{
		e: &Envoy{Bootstrap: testdata.ServerBootstrap},
	}
	var err error
	if err = cs.se.Run(p); err != nil {
		return err
	}
	return nil
}

func (cs *ClientServerEnvoy) Cleanup() {
	cs.ce.Cleanup()
	cs.se.Cleanup()
}
