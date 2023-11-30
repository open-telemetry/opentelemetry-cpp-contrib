/*
* Copyright 2022, OpenTelemetry Authors.  
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef __SDKUTILS_H
#define __SDKUTILS_H

#include <unordered_map>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/nostd/string_view.h>

namespace otel {
namespace core {
namespace sdkwrapper {

using namespace opentelemetry;
class OtelCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
public:
  static constexpr const char* EMPTY_STR = "";
  OtelCarrier(const std::unordered_map<std::string, std::string>& pairs = {}) : kvPairs(pairs) {}

  nostd::string_view Get(nostd::string_view key) const noexcept override
  {
    const std::string keyStr = {key.data(), key.size()};
    auto itr = kvPairs.find(keyStr);
    if (itr != kvPairs.end()) {
      return itr->second;
    }
    return EMPTY_STR;
  }

  void Set(nostd::string_view key, nostd::string_view value) noexcept override
  {
    const std::string keyStr = {key.data(), key.size()};
    const std::string valueStr = {value.data(), value.size()};
    kvPairs[keyStr] = valueStr;
  }

private:
  std::unordered_map<std::string, std::string> kvPairs;
};

} //sdkwrapper
} //core
} //otel

#endif
