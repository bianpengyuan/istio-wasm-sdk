/* Copyright 2020 Istio Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>

namespace Istio {
namespace Extension {
namespace StreamInfo {

enum class ServiceAuthenticationPolicy : int64_t {
  Unspecified = 0,
  None = 1,
  MutualTLS = 2,
};

// StreamInfo lazily load stream related information. It caches the request
// attribute when first time fetching it from the host. Since this is stateful,
// it should only be used for telemetry.
class StreamInfo {
public:
  virtual ~StreamInfo(){};

  virtual int64_t requestTimestamp() = 0;
  virtual int64_t responseTimestamp() = 0;
  virtual int64_t requestSize() = 0;
  virtual int64_t responseSize() = 0;
  virtual int64_t destinationPort() = 0;
  virtual const std::string &sourceAddress() = 0;
  virtual const std::string &destinationAddress() = 0;
  virtual const std::string &requestProtocol() = 0;
  virtual int64_t responseCode() = 0;
  virtual const std::string &responseFlag() = 0;
  virtual const std::string &destinationServiceName() = 0;
  virtual const std::string &destinationServiceHost() = 0;
  virtual const std::string &requestOperation() = 0;
  virtual ServiceAuthenticationPolicy serviceAuthenticationPolicy() = 0;
  virtual const std::string &sourcePrincipal() = 0;
  virtual const std::string &destinationPrincipal() = 0;
  virtual int64_t duration() = 0;
  virtual int64_t responseDuration() = 0;
  virtual const std::string &requestedServerName() = 0;
  virtual bool isOutbound() = 0;
  virtual const std::string &referer() = 0;
  virtual const std::string &userAgent() = 0;
  virtual const std::string &urlPath() = 0;
  virtual const std::string &requestHost() = 0;
  virtual const std::string &requestScheme() = 0;
  virtual const std::string &requestID() = 0;
  virtual const std::string &b3TraceID() = 0;
  virtual const std::string &b3SpanID() = 0;
  virtual bool b3TraceSampled() = 0;
};

} // namespace StreamInfo
} // namespace Extension
} // namespace Istio
