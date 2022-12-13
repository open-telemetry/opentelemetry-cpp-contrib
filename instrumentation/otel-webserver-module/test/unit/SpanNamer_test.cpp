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
#include "gtest/gtest.h"
#include "api/SpanNamer.h"
#include "SpanNamingUtils.h"

TEST(SpanNamer, SpanNamerCreatedWithDefaultValues)
{
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();
    auto spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two");

    spanName = spanNamer->getSpanName("index.html");
    EXPECT_EQ(spanName, "index.html");

    spanName = spanNamer->getSpanName("/");
    EXPECT_EQ(spanName, "/");

    spanName = spanNamer->getSpanName("");
    EXPECT_EQ(spanName, "");
}

TEST(SpanNamer, SpanNamerCreatedWithFirstNSegments)
{
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();

    spanNamer->setSegmentRules("First", "2");
    auto spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two");

    spanNamer->setSegmentRules("First", "15");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two/three/index.html");

    spanNamer->setSegmentRules("First", "0");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two");

    spanNamer->setSegmentRules("Fast", "4");
    spanName = spanNamer->getSpanName("/one/two/three/four/index.html");
    EXPECT_EQ(spanName, "/one/two/three/four");

    spanNamer->setSegmentRules("Fast", "exj");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two");
}

TEST(SpanNamer, SpanNamerCreatedWithLastNSegments)
{
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();

    spanNamer->setSegmentRules("Last", "2");
    auto spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/three/index.html");

    spanNamer->setSegmentRules("Last", "15");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two/three/index.html");

    spanNamer->setSegmentRules("Last", "0");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/three/index.html");

    spanNamer->setSegmentRules("lost", "4");
    spanName = spanNamer->getSpanName("/one/two/three/four/index.html");
    EXPECT_EQ(spanName, "/one/two/three/four");

    spanNamer->setSegmentRules("lost", "exj");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two");

    spanNamer->setSegmentRules("last", "2");
    spanName = spanNamer->getSpanName("index.html");
    EXPECT_EQ(spanName, "index.html");
}

TEST(SpanNamer, SpanNamerCreatedWithCustomSegments)
{
    auto spanNamer = std::make_shared<otel::core::SpanNamer>();

    spanNamer->setSegmentRules("Custom", "2");
    auto spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "two");

    spanNamer->setSegmentRules("Custom", "1,4");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "one/index.html");

    spanNamer->setSegmentRules("Custom", "4,1");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "one/index.html");

    spanNamer->setSegmentRules("Custom", "3,4,5");
    spanName = spanNamer->getSpanName("/one/two/three/four/index.html");
    EXPECT_EQ(spanName, "three/four/index.html");

    spanNamer->setSegmentRules("Custom", "");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two/three/index.html");

    spanNamer->setSegmentRules("Custom", "0,5");
    spanName = spanNamer->getSpanName("index.html");
    EXPECT_EQ(spanName, "index.html");

    spanNamer->setSegmentRules("Custom", "0,2");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "two");

    spanNamer->setSegmentRules("Custom", "0");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two/three/index.html");

    spanNamer->setSegmentRules("Custom", "xyz");
    spanName = spanNamer->getSpanName("/one/two/three/index.html");
    EXPECT_EQ(spanName, "/one/two/three/index.html");

    spanNamer->setSegmentRules("Custom", "0,2,3");
    spanName = spanNamer->getSpanName("/");
    EXPECT_EQ(spanName, "/");
}

