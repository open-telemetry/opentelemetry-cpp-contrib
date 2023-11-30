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
#pragma once

#include <string>

namespace otel {
namespace core {

enum class SegmentType
{
    FIRST,
    LAST,
    CUSTOM
};

enum class SpanNamingRule
{
    URLSEGMENT
};

class SpanNamer
{
public:
    SpanNamer();
    ~SpanNamer() = default;

    void setSegmentRules(const std::string& segmentType, const std::string& segmentParameter);
    std::string getSpanName(const std::string& uri);

private:
    void setSegmentType(const std::string& type);
    void validateAndSetSegmentParameter(const std::string& segmentParameter);

private:
    SegmentType segmentType;
    unsigned segmentCount;
    std::string segmentValues;
};

} // core
} // otel


