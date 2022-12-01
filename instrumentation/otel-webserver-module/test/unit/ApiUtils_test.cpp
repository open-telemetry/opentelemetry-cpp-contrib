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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "api/ApiUtils.h"

TEST(PassedEnvironmentReader, Init_Invalid_env_records)
{
	otel::core::PassedEnvinronmentReader reader;
	auto status = reader.Init(nullptr, 0);
	EXPECT_EQ(status, OTEL_STATUS(environment_records_are_invalid));

	OTEL_SDK_ENV_RECORD* env_config =
                new OTEL_SDK_ENV_RECORD[1] ;
	env_config[0] = OTEL_SDK_ENV_RECORD{"", "dummy_value"};
	status = reader.Init(env_config, 1);
	EXPECT_EQ(status, OTEL_STATUS(environment_record_name_is_not_specified_or_empty));
	delete env_config;

	env_config = new OTEL_SDK_ENV_RECORD[1] ;
	env_config[0] = OTEL_SDK_ENV_RECORD{nullptr, "dummy_value"};
	status = reader.Init(env_config, 1);
	EXPECT_EQ(status, OTEL_STATUS(environment_record_name_is_not_specified_or_empty));
	delete env_config;

	env_config = new OTEL_SDK_ENV_RECORD[1] ;
	env_config[0] = OTEL_SDK_ENV_RECORD{"dummy_key", nullptr};
	status = reader.Init(env_config, 1);
	EXPECT_EQ(status, OTEL_STATUS(environment_record_value_is_not_specified));
	delete env_config;
}

TEST(PassedEnvironmentReader, Init_Valid_env_records)
{
	otel::core::PassedEnvinronmentReader reader;
	OTEL_SDK_ENV_RECORD* env_config =
                new OTEL_SDK_ENV_RECORD[1] ;
	env_config[0] = OTEL_SDK_ENV_RECORD{"dummy_key", ""};
	auto status = reader.Init(env_config, 1);
	EXPECT_EQ(status, OTEL_SUCCESS);
	delete env_config;

	env_config = new OTEL_SDK_ENV_RECORD[1] ;
	env_config[0] = OTEL_SDK_ENV_RECORD{"dummy_key", "dummy_value"};
	status = reader.Init(env_config, 1);
	EXPECT_EQ(status, OTEL_SUCCESS);
	delete env_config;
}

TEST(PassedEnvironmentReader, ReadMandatory)
{
	otel::core::ApiUtils::apiLogger = getLogger("api");
	otel::core::PassedEnvinronmentReader reader;
	OTEL_SDK_ENV_RECORD* env_config = new OTEL_SDK_ENV_RECORD[1] ;
	env_config[0] = OTEL_SDK_ENV_RECORD{"dummy_key", "dummy_value"};
	auto status = reader.Init(env_config, 1);
	EXPECT_EQ(status, OTEL_SUCCESS);

	std::string key = {"dummy_key"};
	std::string value;
	status = reader.ReadMandatory(key, value);
	EXPECT_EQ(status, OTEL_SUCCESS);
	EXPECT_EQ(value, "dummy_value");

	key = "unknown_key";
	std::string value1;
	status = reader.ReadMandatory(key, value1);
	EXPECT_EQ(status, OTEL_STATUS(unspecified_environment_variable));
	delete env_config;

	otel::core::PassedEnvinronmentReader reader2;
	env_config = new OTEL_SDK_ENV_RECORD[1];
	env_config[0] = OTEL_SDK_ENV_RECORD{"dummy_key", ""};
	status = reader2.Init(env_config, 1);
	EXPECT_EQ(status, OTEL_SUCCESS);
	key = "dummy_key";
	value = "";
	status = reader2.ReadMandatory(key, value);
	EXPECT_EQ(status, OTEL_STATUS(environment_variable_invalid_value));
}

TEST(PassedEnvironmentReader, ReadOptional)
{
	otel::core::ApiUtils::apiLogger = getLogger("api");
	otel::core::PassedEnvinronmentReader reader;
	OTEL_SDK_ENV_RECORD* env_config = new OTEL_SDK_ENV_RECORD[1] ;
	env_config[0] = OTEL_SDK_ENV_RECORD{"dummy_key", "dummy_value"};
	auto status = reader.Init(env_config, 1);
	EXPECT_EQ(status, OTEL_SUCCESS);

	std::string key = {"dummy_key"};
	std::string value;
	status = reader.ReadOptional(key, value);
	EXPECT_EQ(status, OTEL_SUCCESS);
	EXPECT_EQ(value, "dummy_value");

	key = "unknown_key";
	std::string value1;
	status = reader.ReadOptional(key, value1);
	EXPECT_EQ(status, OTEL_SUCCESS);
	delete env_config;

	otel::core::PassedEnvinronmentReader reader2;
	env_config = new OTEL_SDK_ENV_RECORD[1];
	env_config[0] = OTEL_SDK_ENV_RECORD{"dummy_key", ""};
	status = reader2.Init(env_config, 1);
	EXPECT_EQ(status, OTEL_SUCCESS);
	key = "dummy_key";
	value = "";
	status = reader2.ReadOptional(key, value);
	EXPECT_EQ(status, OTEL_SUCCESS);

}

TEST(ApiUtils, ReadFromPassedSettings_Success)
{
	OTEL_SDK_ENV_RECORD* env_config = new OTEL_SDK_ENV_RECORD[16];
    int ix = 0;

    // Otel Exporter Type
    env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_TYPE;
    env_config[ix].value = "dummy_exporter";
    ++ix;

    // Otel Exporter Endpoint
    env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_ENDPOINT;
    env_config[ix].value = "dummy_endpoint";
    ++ix;

    // Otel SSL Enabled
    env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_ENABLED;
    env_config[ix].value =  "1";
    ++ix;

    // Otel Certificate Path
    env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH;
    env_config[ix].value = "dummy_path";
    ++ix;

    // sdk libaray name
    env_config[ix].name = OTEL_SDK_ENV_OTEL_LIBRARY_NAME;
    env_config[ix].value = "Apache";
    ++ix;

    // Otel Processor Type
    env_config[ix].name = OTEL_SDK_ENV_OTEL_PROCESSOR_TYPE;
    env_config[ix].value = "dummy_processor";
    ++ix;

    // Otel Sampler Type
    env_config[ix].name = OTEL_SDK_ENV_OTEL_SAMPLER_TYPE;
    env_config[ix].value = "dummy_sampler";
    ++ix;

    // Service Namespace
    env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAMESPACE;
    env_config[ix].value = "dummy_service_namespace";
    ++ix;

    // Service Name
    env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAME;
    env_config[ix].value = "dummy_service";
    ++ix;

    // Service Instance ID
    env_config[ix].name = OTEL_SDK_ENV_SERVICE_INSTANCE_ID;
    env_config[ix].value = "dummy_instance_id";
    ++ix;

    // Otel Max Queue Size
    env_config[ix].name = OTEL_SDK_ENV_MAX_QUEUE_SIZE;
    env_config[ix].value = "2048";
    ++ix;

    // Otel Scheduled Delay
    env_config[ix].name = OTEL_SDK_ENV_SCHEDULED_DELAY;
    env_config[ix].value = "500";
    ++ix;

    // Otel Max Export Batch Size
    env_config[ix].name = OTEL_SDK_ENV_EXPORT_BATCH_SIZE;
    env_config[ix].value = "2048";
    ++ix;

    // Otel Export Timeout
    env_config[ix].name = OTEL_SDK_ENV_EXPORT_TIMEOUT;
    env_config[ix].value = "50000";
    ++ix;

    // Segment Type
    env_config[ix].name = OTEL_SDK_ENV_SEGMENT_TYPE;
    env_config[ix].value = "first";
    ++ix;

    // Segment Parameter
    env_config[ix].name = OTEL_SDK_ENV_SEGMENT_PARAMETER;
    env_config[ix].value = "2";
    ++ix;

    otel::core::SpanNamer spanNamer;
	otel::core::TenantConfig tenantConfig;
	otel::core::ApiUtils apiUtils;

	auto status = apiUtils.ReadFromPassedSettings(env_config, 16, tenantConfig, spanNamer);
	EXPECT_EQ(status, OTEL_SUCCESS);
	delete env_config;
}

TEST(ApiUtils, ReadFromPassedSettings_Fails_On_null_env)
{
	otel::core::SpanNamer spanNamer;
	otel::core::TenantConfig tenantConfig;
	otel::core::ApiUtils apiUtils;
	auto status = apiUtils.ReadFromPassedSettings(nullptr, 0, tenantConfig, spanNamer);
	EXPECT_NE(status, OTEL_SUCCESS);
}

TEST(ApiUtils, ReadFromPassedSettings_Failure_On_Invalid_Int)
{
	OTEL_SDK_ENV_RECORD* env_config = new OTEL_SDK_ENV_RECORD[16];
    int ix = 0;

    // Otel Exporter Type
    env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_TYPE;
    env_config[ix].value = "dummy_exporter";
    ++ix;

    // Otel Exporter Endpoint
    env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_ENDPOINT;
    env_config[ix].value = "dummy_endpoint";
    ++ix;

    // Otel SSL Enabled
    env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_ENABLED;
    env_config[ix].value =  "1";
    ++ix;

    // Otel Certificate Path
    env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH;
    env_config[ix].value = "dummy_path";
    ++ix;

    // sdk libaray name
    env_config[ix].name = OTEL_SDK_ENV_OTEL_LIBRARY_NAME;
    env_config[ix].value = "Apache";
    ++ix;

    // Otel Processor Type
    env_config[ix].name = OTEL_SDK_ENV_OTEL_PROCESSOR_TYPE;
    env_config[ix].value = "dummy_processor";
    ++ix;

    // Otel Sampler Type
    env_config[ix].name = OTEL_SDK_ENV_OTEL_SAMPLER_TYPE;
    env_config[ix].value = "dummy_sampler";
    ++ix;

    // Service Namespace
    env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAMESPACE;
    env_config[ix].value = "dummy_service_namespace";
    ++ix;

    // Service Name
    env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAME;
    env_config[ix].value = "dummy_service";
    ++ix;

    // Service Instance ID
    env_config[ix].name = OTEL_SDK_ENV_SERVICE_INSTANCE_ID;
    env_config[ix].value = "dummy_instance_id";
    ++ix;

    // Otel Max Queue Size
    env_config[ix].name = OTEL_SDK_ENV_MAX_QUEUE_SIZE;
    env_config[ix].value = "2048";
    ++ix;

    // Otel Scheduled Delay
    env_config[ix].name = OTEL_SDK_ENV_SCHEDULED_DELAY;
    env_config[ix].value = "500";
    ++ix;

    // Otel Max Export Batch Size
    env_config[ix].name = OTEL_SDK_ENV_EXPORT_BATCH_SIZE;
    env_config[ix].value = "abcd";
    ++ix;

    // Otel Export Timeout
    env_config[ix].name = OTEL_SDK_ENV_EXPORT_TIMEOUT;
    env_config[ix].value = "50000";
    ++ix;

    // Segment Type
    env_config[ix].name = OTEL_SDK_ENV_SEGMENT_TYPE;
    env_config[ix].value = "first";
    ++ix;

    // Segment Parameter
    env_config[ix].name = OTEL_SDK_ENV_SEGMENT_PARAMETER;
    env_config[ix].value = "2";
    ++ix;

    otel::core::SpanNamer spanNamer;
	otel::core::TenantConfig tenantConfig;
	otel::core::ApiUtils apiUtils;

	auto status = apiUtils.ReadFromPassedSettings(env_config, 16, tenantConfig, spanNamer);
	EXPECT_EQ(status, OTEL_STATUS(environment_variable_invalid_value));
	delete env_config;
}

TEST(ApiUtils, ReadFromPassedSettings_Failure_On_Invalid_Bool)
{
	OTEL_SDK_ENV_RECORD* env_config = new OTEL_SDK_ENV_RECORD[16];
    int ix = 0;

    // Otel Exporter Type
    env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_TYPE;
    env_config[ix].value = "dummy_exporter";
    ++ix;

    // Otel Exporter Endpoint
    env_config[ix].name = OTEL_SDK_ENV_OTEL_EXPORTER_ENDPOINT;
    env_config[ix].value = "dummy_endpoint";
    ++ix;

    // Otel SSL Enabled
    env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_ENABLED;
    env_config[ix].value =  "abcd";
    ++ix;

    // Otel Certificate Path
    env_config[ix].name = OTEL_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH;
    env_config[ix].value = "dummy_path";
    ++ix;

    // sdk libaray name
    env_config[ix].name = OTEL_SDK_ENV_OTEL_LIBRARY_NAME;
    env_config[ix].value = "Apache";
    ++ix;

    // Otel Processor Type
    env_config[ix].name = OTEL_SDK_ENV_OTEL_PROCESSOR_TYPE;
    env_config[ix].value = "dummy_processor";
    ++ix;

    // Otel Sampler Type
    env_config[ix].name = OTEL_SDK_ENV_OTEL_SAMPLER_TYPE;
    env_config[ix].value = "dummy_sampler";
    ++ix;

    // Service Namespace
    env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAMESPACE;
    env_config[ix].value = "dummy_service_namespace";
    ++ix;

    // Service Name
    env_config[ix].name = OTEL_SDK_ENV_SERVICE_NAME;
    env_config[ix].value = "dummy_service";
    ++ix;

    // Service Instance ID
    env_config[ix].name = OTEL_SDK_ENV_SERVICE_INSTANCE_ID;
    env_config[ix].value = "dummy_instance_id";
    ++ix;

    // Otel Max Queue Size
    env_config[ix].name = OTEL_SDK_ENV_MAX_QUEUE_SIZE;
    env_config[ix].value = "2048";
    ++ix;

    // Otel Scheduled Delay
    env_config[ix].name = OTEL_SDK_ENV_SCHEDULED_DELAY;
    env_config[ix].value = "500";
    ++ix;

    // Otel Max Export Batch Size
    env_config[ix].name = OTEL_SDK_ENV_EXPORT_BATCH_SIZE;
    env_config[ix].value = "2048";
    ++ix;

    // Otel Export Timeout
    env_config[ix].name = OTEL_SDK_ENV_EXPORT_TIMEOUT;
    env_config[ix].value = "50000";
    ++ix;

    // Segment Type
    env_config[ix].name = OTEL_SDK_ENV_SEGMENT_TYPE;
    env_config[ix].value = "first";
    ++ix;

    // Segment Parameter
    env_config[ix].name = OTEL_SDK_ENV_SEGMENT_PARAMETER;
    env_config[ix].value = "2";
    ++ix;

    otel::core::SpanNamer spanNamer;
	otel::core::TenantConfig tenantConfig;
	otel::core::ApiUtils apiUtils;

	auto status = apiUtils.ReadFromPassedSettings(env_config, 16, tenantConfig, spanNamer);
	EXPECT_EQ(status, OTEL_STATUS(environment_variable_invalid_value));
	delete env_config;
}

TEST(ApiUtils, getSDKInstallPath)
{
	otel::core::ApiUtils apiUtils;
	auto path = apiUtils.getSDKInstallPath();
	EXPECT_THAT(path.string(), testing::EndsWith("build/linux-x64/opentelemetry-webserver-sdk"));
}
