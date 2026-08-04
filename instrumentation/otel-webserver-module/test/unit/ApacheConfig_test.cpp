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

#include <apr_pools.h>
#include <apr_strings.h>
#include <string.h>

#include "ApacheConfig.h"
#include "ApacheHooks.h"
#include "ApacheTracing.h"

bool ApacheTracing::m_traceAsErrorFromUser = false;
void ApacheTracing::writeTrace(server_rec*, const char*, const char*, ...) {}
void ApacheTracing::writeError(server_rec*, const char*, const char*, ...) {}
bool ApacheHooks::m_reportAllStages = false;
otel_cfg* ApacheConfigHandlers::our_dconfig(const request_rec*) { return NULL; }

namespace {

class ApacheConfigMergeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(APR_SUCCESS, apr_initialize());
        ASSERT_EQ(APR_SUCCESS, apr_pool_create(&m_pool, NULL));

        memset(&m_cmd, 0, sizeof(m_cmd));
        m_cmd.pool = m_pool;
        m_cmd.server = NULL;
    }

    void TearDown() override
    {
        apr_pool_destroy(m_pool);
        apr_terminate();
    }

    otel_cfg* createConfig(const char* name)
    {
        return static_cast<otel_cfg*>(
            ApacheConfigHandlers::otel_create_dir_config(m_pool, const_cast<char*>(name)));
    }

    void setHeaders(otel_cfg* cfg, const char* value)
    {
        ApacheConfigHandlers::otel_set_otelExporterOtlpHeaders(&m_cmd, cfg, value);
    }

    otel_cfg* merge(otel_cfg* parent, otel_cfg* child)
    {
        return static_cast<otel_cfg*>(
            ApacheConfigHandlers::otel_merge_dir_config(m_pool, parent, child));
    }

    apr_pool_t* m_pool = NULL;
    cmd_parms m_cmd;
};

TEST_F(ApacheConfigMergeTest, OtlpHeadersInheritedFromParentWhenChildUnset)
{
    otel_cfg* parent = createConfig("server");
    setHeaders(parent, "api-key=parentkey");

    otel_cfg* child = createConfig("/var/www/");

    otel_cfg* merged = merge(parent, child);

    ASSERT_NE(nullptr, merged->getOtelExporterOtlpHeaders());
    EXPECT_STREQ("api-key=parentkey", merged->getOtelExporterOtlpHeaders());
    EXPECT_EQ(1, merged->otelExporterOtlpHeadersInitialized());
}

TEST_F(ApacheConfigMergeTest, OtlpHeadersOverriddenByChildWhenSet)
{
    otel_cfg* parent = createConfig("server");
    setHeaders(parent, "api-key=parentkey");

    otel_cfg* child = createConfig("/var/www/secure/");
    setHeaders(child, "api-key=childkey");

    otel_cfg* merged = merge(parent, child);

    ASSERT_NE(nullptr, merged->getOtelExporterOtlpHeaders());
    EXPECT_STREQ("api-key=childkey", merged->getOtelExporterOtlpHeaders());
}

TEST_F(ApacheConfigMergeTest, OtlpHeadersAreCopiedNotAliased)
{
    otel_cfg* parent = createConfig("server");
    setHeaders(parent, "api-key=parentkey");

    otel_cfg* child = createConfig("/var/www/");

    otel_cfg* merged = merge(parent, child);

    EXPECT_NE(parent->getOtelExporterOtlpHeaders(), merged->getOtelExporterOtlpHeaders());
    EXPECT_STREQ(parent->getOtelExporterOtlpHeaders(), merged->getOtelExporterOtlpHeaders());
}

TEST_F(ApacheConfigMergeTest, MergeLeavesNoFieldUninitialized)
{
    otel_cfg* parent = createConfig("server");
    otel_cfg* child = createConfig("/var/www/");

    otel_cfg* merged = merge(parent, child);

    EXPECT_EQ(1, merged->getOtelEnabledInitialized());
    EXPECT_EQ(1, merged->getOtelExporterTypeInitialized());
    EXPECT_EQ(1, merged->getOtelExporterEndpointInitialized());
    EXPECT_EQ(1, merged->otelExporterOtlpHeadersInitialized());
    EXPECT_EQ(1, merged->getOtelSslEnabledInitialized());
    EXPECT_EQ(1, merged->getOtelSslCertificatePathInitialized());
    EXPECT_EQ(1, merged->getOtelProcessorTypeInitialized());
    EXPECT_EQ(1, merged->getOtelSamplerTypeInitialized());
    EXPECT_EQ(1, merged->getServiceNamespaceInitialized());
    EXPECT_EQ(1, merged->getServiceNameInitialized());
    EXPECT_EQ(1, merged->getServiceInstanceIdInitialized());
    EXPECT_EQ(1, merged->getOtelMaxQueueSizeInitialized());
    EXPECT_EQ(1, merged->getOtelScheduledDelayInitialized());
    EXPECT_EQ(1, merged->getOtelExportTimeoutInitialized());
    EXPECT_EQ(1, merged->getOtelMaxExportBatchSizeInitialized());
    EXPECT_EQ(1, merged->getResolveBackendsInitialized());
    EXPECT_EQ(1, merged->getTraceAsErrorInitialized());
    EXPECT_EQ(1, merged->getReportAllInstrumentedModulesInitialized());
    EXPECT_EQ(1, merged->getMaskCookieInitialized());
    EXPECT_EQ(1, merged->getCookiePatternInitialized());
    EXPECT_EQ(1, merged->getMaskSmUserInitialized());
    EXPECT_EQ(1, merged->getDelimiterInitialized());
    EXPECT_EQ(1, merged->getSegmentInitialized());
    EXPECT_EQ(1, merged->getMatchFilterInitialized());
    EXPECT_EQ(1, merged->getMatchPatternInitialized());
    EXPECT_EQ(1, merged->getSegmentTypeInitialized());
    EXPECT_EQ(1, merged->getSegmentParameterInitialized());
}

}  // namespace
