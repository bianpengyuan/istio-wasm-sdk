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
	"errors"
	"fmt"
	"log"
	"sort"
	"strings"
	"time"

	"github.com/d4l3k/messagediff"
	dto "github.com/prometheus/client_model/go"
	"github.com/prometheus/common/expfmt"
)

type Stats struct {
	AdminPort uint16
	Matchers  map[string]StatMatcher
}

type StatMatcher interface {
	Matches(*Params, *dto.MetricFamily) error
}

var _ Step = &Stats{}

func (s *Stats) Run(p *Params) error {
	for i := 0; i < 15; i++ {
		_, _, body, err := httpGet(fmt.Sprintf("http://127.0.0.1:%d/stats/prometheus", s.AdminPort), map[string][]string{})
		if err != nil {
			return err
		}
		reader := strings.NewReader(body)
		metrics, err := (&expfmt.TextParser{}).TextToMetricFamilies(reader)
		if err != nil {
			return err
		}
		count := 0
		for _, metric := range metrics {
			matcher, found := s.Matchers[metric.GetName()]
			if !found {
				continue
			}
			if err := matcher.Matches(p, metric); err == nil {
				log.Printf("matched metric %q", metric.GetName())
				count++
			} else {
				log.Printf("metric %q did not match: %v\n", metric.GetName(), err)
			}
		}
		if count == len(s.Matchers) {
			return nil
		}
		log.Printf("failed to match all metrics: want %#v", s.Matchers)
		time.Sleep(1 * time.Second)
	}
	return errors.New("failed to match all stats")
}

func (s *Stats) Cleanup() {}

type ExactStat struct {
	Metric string
}

func (me *ExactStat) Matches(params *Params, that *dto.MetricFamily) error {
	metric := &dto.MetricFamily{}
	params.LoadTestProto(me.Metric, metric)
	// sort labels based on name
	for _, m := range that.Metric {
		sort.SliceStable(m.Label, func(i, j int) bool {
			return *m.Label[i].Name < *m.Label[j].Name
		})
	}
	// sort metrics based on label pairs
	sort.SliceStable(that.Metric, func(i, j int) bool {
		for ind, l := range that.Metric[i].Label {
			if ind >= len(that.Metric[j].Label) {
				return false
			}
			n1 := *l.Name
			v1 := *l.Value
			n2 := *that.Metric[j].Label[ind].Name
			v2 := *that.Metric[j].Label[ind].Value

			if n1 < n2 {
				return true
			}
			if n1 > n2 {
				return false
			}
			if v1 < v2 {
				return true
			}
			if v1 > n2 {
				return false
			}
		}
		return true
	})
	if s, same := messagediff.PrettyDiff(metric, that); !same {
		return fmt.Errorf("diff: %v, got: %v, want: %v", s, that, metric)
	}
	return nil
}

var _ StatMatcher = &ExactStat{}
