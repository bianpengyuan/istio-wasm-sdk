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
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"time"
	"encoding/json"
	"net/http"
	"io/ioutil"
)

// Envoy stores data for Envoy process
type Envoy struct {
	cmd    *exec.Cmd
	Ports  *Ports
	baseID string
}

// NewClientEnvoy creates a new Client Envoy struct and starts envoy.
func (s *TestSetup) NewClientEnvoy() (*Envoy, error) {
	confTmpl := envoyClientConfTemplYAML
	if s.tec.ClientEnvoyTemplate != "" {
		confTmpl = s.tec.ClientEnvoyTemplate
	}
	baseID := strconv.Itoa(int(s.testName)*2 + 1)

	return newEnvoy(s.tec.Ports.ClientAdminPort, confTmpl, baseID, "client.yaml", s)
}

// NewServerEnvoy creates a new Server Envoy struct and starts envoy.
func (s *TestSetup) NewServerEnvoy() (*Envoy, error) {
	confTmpl := envoyServerConfTemplYAML
	if s.tec.ServerEnvoyTemplate != "" {
		confTmpl = s.tec.ServerEnvoyTemplate
	}
	baseID := strconv.Itoa(int(s.testName+1) * 2)

	return newEnvoy(s.tec.Ports.ServerAdminPort, confTmpl, baseID, "server.yaml", s)
}

// Start starts the envoy process
func (s *Envoy) Start(port uint16) error {
	log.Printf("server cmd %v", s.cmd.Args)
	err := s.cmd.Start()
	if err != nil {
		return err
	}

	url := fmt.Sprintf("http://localhost:%v/server_info", port)
	return WaitForHTTPServer(url)
}

// Stop stops the envoy process
func (s *Envoy) Stop(port uint16) error {
	log.Printf("stop envoy ...\n")
	_, _, _ = HTTPPost(fmt.Sprintf("http://127.0.0.1:%v/quitquitquit", port), "", "")
	done := make(chan error, 1)
	go func() {
		done <- s.cmd.Wait()
	}()

	select {
	case <-time.After(3 * time.Second):
		log.Println("envoy killed as timeout reached")
		if err := s.cmd.Process.Kill(); err != nil {
			return err
		}
	case err := <-done:
		log.Printf("stop envoy ... done\n")
		return err
	}

	return nil
}

// TearDown removes shared memory left by Envoy
func (s *Envoy) TearDown() {
	if s.baseID != "" {
		path := "/dev/shm/envoy_shared_memory_" + s.baseID + "0"
		if err := os.Remove(path); err != nil {
			log.Printf("failed to %s\n", err)
		} else {
			log.Printf("removed Envoy's shared memory\n")
		}
	}
}

func copyYamlFiles(src, dst string) {
	cpCmd := exec.Command("cp", "-rf", src, dst)
	if err := cpCmd.Run(); err != nil {
		log.Printf("Error Copying Yaml Files %s\n", err)
	}
}

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

	dst := fmt.Sprintf("istio-proxy/envoy-%v", proxySHA)
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

	mkdirCmd := exec.Command("mkdir", "istio-proxy")
	if err := mkdirCmd.Run(); err != nil {
		return "", fmt.Errorf("fail to create directory for %v: %v", dst, err)
	}
	cpCmd := exec.Command("cp", src, dst)
	if err := cpCmd.Run(); err != nil {
		return "", fmt.Errorf("fail to copy envoy binary from %v to %v: %v", src, dst, err)
	}
	
	return dst, nil
}

// NewEnvoy creates a new Envoy struct and starts envoy at the specified port.
func newEnvoy(port uint16, confTmpl, baseID, yamlName string, s *TestSetup) (*Envoy, error) {
	confPath := filepath.Join(GetDefaultIstioOut(), fmt.Sprintf("config.conf.%v.yaml", port))
	log.Printf("Envoy config: in %v\n", confPath)
	if err := s.CreateEnvoyConf(confPath, confTmpl); err != nil {
		return nil, err
	}

	if s.copyYamlFiles {
		if wd, err := os.Getwd(); err == nil {
			if err := os.MkdirAll(filepath.Join(wd, "testoutput"), os.ModePerm); err == nil {
				copyYamlFiles(confPath, filepath.Join(wd, "testoutput", yamlName))
			}
		}
	}

	debugLevel, ok := os.LookupEnv("ENVOY_DEBUG")
	if !ok {
		debugLevel = "info"
	}

	args := []string{"-c", confPath,
		"--drain-time-s", "1",
		"--allow-unknown-fields"}
	if s.stress {
		args = append(args, "--concurrency", "10")
	} else {
		// debug is far too verbose.
		args = append(args, "-l", debugLevel, "--concurrency", "1")
	}
	if s.disableHotRestart {
		args = append(args, "--disable-hot-restart")
	} else {
		args = append(args,
			// base id is shared between restarted envoys
			"--base-id", baseID,
			"--parent-shutdown-time-s", "1",
			"--restart-epoch", strconv.Itoa(s.epoch))
	}
	if s.tec.EnvoyParams != nil {
		args = append(args, s.tec.EnvoyParams...)
	}

	// Download the test envoy binary. Right now only download one version
	envoyPath, err := downloadEnvoy("release-1.5")
	if err != nil {
		return nil, err
	}

	cmd := exec.Command(envoyPath, args...)
	cmd.Stderr = os.Stderr
	cmd.Stdout = os.Stdout
	if s.Dir != "" {
		cmd.Dir = s.Dir
	}
	return &Envoy{
		cmd:    cmd,
		Ports:  s.tec.Ports,
		baseID: baseID,
	}, nil
}
