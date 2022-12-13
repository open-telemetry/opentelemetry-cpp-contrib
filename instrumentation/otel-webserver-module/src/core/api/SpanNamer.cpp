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
#include "api/SpanNamer.h"
#include "api/ApiUtils.h"
#include "SpanNamingUtils.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace otel {
namespace core {

const unsigned DefaultSegmentCount = 2;
const std::string SegmentSeparator = "/";

SpanNamer::SpanNamer()
    : segmentType(SegmentType::FIRST)
    , segmentCount(DefaultSegmentCount)
{
}

void
SpanNamer::setSegmentRules(const std::string& segmentType,
    const std::string& segmentParameter) {
    setSegmentType(segmentType);
    validateAndSetSegmentParameter(segmentParameter);
}

std::string
SpanNamer::getSpanName(const std::string& uri) {
    std::string spanName;
    switch (segmentType) {
        case SegmentType::FIRST:
            spanName = getFirstNSegments(uri, segmentCount);
            break;
        case SegmentType::LAST:
            spanName = getLastNSegments(uri, segmentCount);
            break;
        case SegmentType::CUSTOM:
            spanName = transformNameWithURISegments(
                uri, uri, segmentValues, SegmentSeparator);
            break;
    }
    return spanName;
}

void
SpanNamer::setSegmentType(const std::string& type) {
    std::string lcType = type;
    std::transform(lcType.begin(), lcType.end(), lcType.begin(), ::tolower);

    if (lcType == "last") {
        segmentType = SegmentType::LAST;
    } else if (lcType == "custom") {
        segmentType = SegmentType::CUSTOM;
    } else {
        segmentType = SegmentType::FIRST;
    }
}

void
SpanNamer::validateAndSetSegmentParameter(const std::string& segmentParameter) {
    bool isInvalidValue = false;
    switch(segmentType) {
        case SegmentType::FIRST:
        case SegmentType::LAST:
            try {
                segmentCount = boost::lexical_cast<unsigned int>(segmentParameter);
            } catch(const boost::bad_lexical_cast& e) {
                /*LOG4CXX_ERROR(  ApiUtils::apiLogger,
                        boost::format("Invalid Value %1% specified for SegmentParameter: Error")
                            % segmentParameter
                            % e.target_type().name());*/
                // Setting to default values
                segmentCount = DefaultSegmentCount;
            }
            // Zero is invalid value, set it to default 2.
            segmentCount = segmentCount == 0 ? DefaultSegmentCount : segmentCount;
            break;
        case SegmentType::CUSTOM:
            segmentValues = segmentParameter;
            break;
    }
}

}
}
