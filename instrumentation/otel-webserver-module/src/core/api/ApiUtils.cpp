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

// file header
#include "api/ApiUtils.h"
#include "api/OpentelemetrySdk.h"
#include "AgentLogger.h"
#include <api/TenantConfig.h>
#include <boost/lexical_cast.hpp>
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace otel {
namespace core {

AgentLogger ApiUtils::apiLogger = 0;
AgentLogger ApiUtils::apiUserLogger = 0;

void ApiUtils::cleanup()
{
    apiLogger = 0;
    apiUserLogger = 0;
}

OTEL_SDK_STATUS_CODE ApiUtils::init_boilerplate()
{
    try
    {
        boost::filesystem::path logConfigPath;

        char* envLogConfigPath = getenv(OTEL_SDK_ENV_LOG_CONFIG_PATH);
        if(envLogConfigPath)
        {
            logConfigPath = envLogConfigPath;
        }
        else
        {
            logConfigPath =
                    getSDKInstallPath()
                    / boost::filesystem::path("conf")
                    / boost::filesystem::path("opentelemetry_sdk_log4cxx.xml");
        }

        boost::system::error_code ec;
        if (!boost::filesystem::exists(logConfigPath, ec)) // no throw version of exists()
        {
            std::cerr << (boost::format( "Error: %1%: Invalid logging config file: %2%")
                    % BOOST_CURRENT_FUNCTION % logConfigPath) << std::endl;
            return OTEL_STATUS(no_log_config);
        }

        bool res = initLogging(logConfigPath);
        if(!res)
        {
            return OTEL_STATUS(log_init_failed);
        }

        ApiUtils::apiLogger = getLogger(OTEL_LOG_API_LOGGER);
        if(!ApiUtils::apiLogger)
        {
            return OTEL_STATUS(log_init_failed);
        }

        LOG4CXX_INFO(ApiUtils::apiLogger,
        	"API logger initialized using log configuration file: " << logConfigPath.string());

        ApiUtils::apiUserLogger = getLogger(OTEL_LOG_API_USER_LOGGER);
        if(!ApiUtils::apiUserLogger)
        {
            return OTEL_STATUS(log_init_failed);
        }
        LOG4CXX_INFO(ApiUtils::apiUserLogger,
        	"API User logger initialized using log configuration file:" << logConfigPath.string());

        std::atexit(cleanup);
    }
    catch(...)
    {
        return OTEL_STATUS(fail);
    }

    return OTEL_SUCCESS;
}

boost::filesystem::path ApiUtils::getSDKInstallPath()
{
#ifdef _WIN32
    char path[FILENAME_MAX];
    HMODULE hm = NULL;
    if (!(hm = GetModuleHandleA("opentelemetry_webserver_sdk")))
    {
        int ret = GetLastError();
        // Logger not initialized it
        fprintf(stderr, "GetModuleHandle returned %d\n", ret);
    }
    GetModuleFileNameA(hm, path, sizeof(path));
    boost::filesystem::path SOpath(path);
#else
    Dl_info dl_info = { 0 };
    dladdr((void*)ApiUtils::getSDKInstallPath, &dl_info);

    boost::filesystem::path SOpath(dl_info.dli_fname);
#endif

    if (!boost::filesystem::exists(SOpath))
    {
        std::cerr << (boost::format("Error: %1%: Invalid shared library path: %2%")
                % BOOST_CURRENT_FUNCTION % SOpath) << std::endl;
        return boost::filesystem::path();
    }

    boost::filesystem::path installPath = SOpath.parent_path().parent_path().parent_path();
    if (!boost::filesystem::exists(installPath))
    {
        std::cerr << (boost::format("Error: %1%: Cannot get install path from shared library path: %2%")
                % BOOST_CURRENT_FUNCTION % SOpath) << std::endl;
        return boost::filesystem::path();
    }

    installPath = boost::filesystem::canonical(installPath);
    return installPath;
}

OTEL_SDK_STATUS_CODE ApiUtils::ReadFromPassedSettings(
        OTEL_SDK_ENV_RECORD* envIn,
        unsigned numberOfRecords,
        TenantConfig& tenantConfig,
        SpanNamer& spanNamer)
{
    PassedEnvinronmentReader env;
    OTEL_SDK_STATUS_CODE res = env.Init(envIn, numberOfRecords);
    if(OTEL_ISFAIL(res))
    {
        return res;
    }

    return ReadSettingsFromReader(env, tenantConfig, spanNamer);
}

/*OTEL_SDK_STATUS_CODE ApiUtils::ReadFromEnvinronment(TenantConfig& tenantConfig)
{
    RealEnvinronmentReader env;
    return ReadSettingsFromReader(env, tenantConfig);
}*/

OTEL_SDK_STATUS_CODE ApiUtils::ReadSettingsFromReader(
    IEnvReader& reader, TenantConfig& tenantConfig,
    SpanNamer& spanNamer)
{
    std::string serviceNamespace;
    std::string serviceName;
    std::string serviceInstanceId;

    std::string otelExporterType;
    std::string otelExporterEndpoint;
    std::string otelExporterOtlpHeaders;
    bool otelSslEnabled;
    std::string otelSslCertPath;
    std::string otelLibraryName;
    std::string otelProcessorType;
    std::string otelSamplerType;

    unsigned otelMaxQueueSize;
    unsigned otelScheduledDelayMillis;
    //unsigned otelExportTimeoutMillis;
    unsigned otelMaxExportBatchSize;

    std::string segmentType;
    std::string segmentParameter;

    OTEL_SDK_STATUS_CODE status;

    status = reader.ReadMandatory(
        std::string(OTEL_SDK_ENV_SERVICE_NAMESPACE), serviceNamespace);
    if(OTEL_ISFAIL(status))
        return status;

    status = reader.ReadMandatory(
        std::string(OTEL_SDK_ENV_SERVICE_NAME), serviceName);
    if(OTEL_ISFAIL(status))
        return status;

    status = reader.ReadMandatory(
        std::string(OTEL_SDK_ENV_SERVICE_INSTANCE_ID), serviceInstanceId);
    if(OTEL_ISFAIL(status))
        return status;

    status = reader.ReadOptional(
        std::string(OTEL_SDK_ENV_OTEL_EXPORTER_TYPE), otelExporterType);
    if(OTEL_ISFAIL(status))
        return status;

    status = reader.ReadMandatory(
        std::string(OTEL_SDK_ENV_OTEL_EXPORTER_ENDPOINT), otelExporterEndpoint);
    if(OTEL_ISFAIL(status))
        return status;

    status = ReadOptionalFromReader(
        reader, std::string(OTEL_SDK_ENV_OTEL_SSL_ENABLED), otelSslEnabled);
    if(OTEL_ISFAIL(status))
        return status;

    status = reader.ReadOptional(
        std::string(OTEL_SDK_ENV_OTEL_SSL_CERTIFICATE_PATH), otelSslCertPath);
    if(OTEL_ISFAIL(status))
        return status;

    status = reader.ReadMandatory(
        std::string(OTEL_SDK_ENV_OTEL_LIBRARY_NAME), otelLibraryName);
    if(OTEL_ISFAIL(status))
        return status;

    reader.ReadOptional(
        std::string(OTEL_SDK_ENV_OTEL_PROCESSOR_TYPE), otelProcessorType);

    reader.ReadOptional(
        std::string(OTEL_SDK_ENV_OTEL_SAMPLER_TYPE), otelSamplerType);

    status = ReadOptionalFromReader(
        reader, std::string(OTEL_SDK_ENV_MAX_QUEUE_SIZE), otelMaxQueueSize);
    if(OTEL_ISFAIL(status))
        return status;

    status = ReadOptionalFromReader(
        reader, std::string(OTEL_SDK_ENV_SCHEDULED_DELAY), otelScheduledDelayMillis);
    if(OTEL_ISFAIL(status))
        return status;

    status = ReadOptionalFromReader(
        reader, std::string(OTEL_SDK_ENV_EXPORT_BATCH_SIZE), otelMaxExportBatchSize);
    if(OTEL_ISFAIL(status))
        return status;

    /*status = reader.ReadOptionalFromReader(
        reader, std::string(OTEL_SDK_ENV_EXPORT_BATCH_SIZE), setOtelExportTimeoutMillis);
    if(OTEL_ISFAIL(status))
        return status;*/

    reader.ReadOptional(
        std::string(OTEL_SDK_ENV_SEGMENT_TYPE), segmentType);

    reader.ReadOptional(
        std::string(OTEL_SDK_ENV_SEGMENT_PARAMETER), segmentParameter);

    reader.ReadOptional(
            std::string(OTEL_SDK_ENV_OTEL_EXPORTER_OTLPHEADERS), otelExporterOtlpHeaders);


    tenantConfig.setServiceNamespace(serviceNamespace);
    tenantConfig.setServiceName(serviceName);
    tenantConfig.setServiceInstanceId(serviceInstanceId);
    tenantConfig.setOtelExporterType(otelExporterType);
    tenantConfig.setOtelExporterEndpoint(otelExporterEndpoint);
    tenantConfig.setOtelExporterOtlpHeaders(otelExporterOtlpHeaders);
    tenantConfig.setOtelLibraryName(otelLibraryName);
    tenantConfig.setOtelProcessorType(otelProcessorType);
    tenantConfig.setOtelSamplerType(otelSamplerType);
    tenantConfig.setOtelMaxQueueSize(otelMaxQueueSize);
    tenantConfig.setOtelScheduledDelayMillis(otelScheduledDelayMillis);
    tenantConfig.setOtelMaxExportBatchSize(otelMaxExportBatchSize);
    //tenantConfig.setOtelMaxExportBatchSize(setOtelExportTimeoutMillis);
    tenantConfig.setOtelSslEnabled(otelSslEnabled);
    tenantConfig.setOtelSslCertPath(otelSslCertPath);

    spanNamer.setSegmentRules(segmentType, segmentParameter);

    return OTEL_SUCCESS;
}

OTEL_SDK_STATUS_CODE ApiUtils::ReadOptionalFromReader(
	IEnvReader& reader, const std::string& varName, bool& result)
{
    std::string value;
    OTEL_SDK_STATUS_CODE status = reader.ReadOptional(varName, value);
    if (value.empty())
    {
        return OTEL_SUCCESS;
    }

    try
    {
        result = boost::lexical_cast<bool>(value);
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG4CXX_ERROR(  ApiUtils::apiLogger,
                        boost::format("Environment variable %1% specified in wrong format: failed to cast %2% to %3% type")
                            % varName.c_str()
                            % value
                            % e.target_type().name());

        return OTEL_STATUS(environment_variable_invalid_value);
    }


    return OTEL_SUCCESS;
}

/*OTEL_SDK_STATUS_CODE ApiUtils::ReadMandatoryFromReader(
	IEnvReader& reader, const std::string& varName, unsigned short& result)
{
    std::string value;
    OTEL_SDK_STATUS_CODE status = reader.ReadMandatory(varName, value);

    if (OTEL_ISFAIL(status))
    {
        return status;
    }

    try
    {
        result = boost::lexical_cast<unsigned short>(value);
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG4CXX_ERROR(  ApiUtils::apiLogger,
                        boost::format("Environment variable %1% specified in wrong format: failed to cast %2% to %3% type")
                            % varName.c_str()
                            % value
                            % e.target_type().name());

        return OTEL_STATUS(environment_variable_invalid_value);
    }

    return OTEL_SUCCESS;
}*/

OTEL_SDK_STATUS_CODE ApiUtils::ReadOptionalFromReader(
	IEnvReader& reader, const std::string& varName, unsigned int& result)
{
    std::string value;
    OTEL_SDK_STATUS_CODE status = reader.ReadOptional(varName, value);

    if (value.empty())
    {
        return OTEL_SUCCESS;
    }

    try
    {
        result = boost::lexical_cast<unsigned int>(value);
    }
    catch (const boost::bad_lexical_cast& e)
    {
        LOG4CXX_ERROR(  ApiUtils::apiLogger,
                        boost::format("Environment variable %1% specified in wrong format: failed to cast %2% to %3% type")
                            % varName.c_str()
                            % value
                            % e.target_type().name());

        return OTEL_STATUS(environment_variable_invalid_value);
    }

    return OTEL_SUCCESS;
}

/*OTEL_SDK_STATUS_CODE RealEnvinronmentReader::ReadMandatory(
	const std::string& varName, std::string& result)
{
    result.clear();

    char* pValue = getenv(varName.c_str());
    if(!pValue)
    {
        LOG4CXX_ERROR(ApiUtils::apiLogger, boost::format("Environment variable %1% must be specified") % varName.c_str());
        return OTEL_STATUS(unspecified_environment_variable);
    }

    std::string value = pValue;
    if(value.empty())
    {
        LOG4CXX_ERROR(ApiUtils::apiLogger, boost::format("Environment variable %1% must be non-empty") % varName.c_str());
        return OTEL_STATUS(unspecified_environment_variable);
    }

    result = value;

    return OTEL_SUCCESS;
}

OTEL_SDK_STATUS_CODE RealEnvinronmentReader::ReadOptional(
	const std::string& varName, std::string& result)
{
    result.clear();

    char* varValue = getenv(varName.c_str());
    if(!varValue)
    {
        LOG4CXX_TRACE(ApiUtils::apiLogger, boost::format("Environment variable %1% is not specified") % varName.c_str());
        return OTEL_SUCCESS;
    }

    std::string value = varValue;
    if(value.empty())
    {
        LOG4CXX_TRACE(ApiUtils::apiLogger, boost::format("Environment variable %1% is non-empty") % varName.c_str());
        return OTEL_SUCCESS;
    }

    result = value;

    return OTEL_SUCCESS;
}*/

OTEL_SDK_STATUS_CODE PassedEnvinronmentReader::Init(
	OTEL_SDK_ENV_RECORD* envIn, unsigned numberOfRecords)
{
    if(!envIn)
    {
        return OTEL_STATUS(environment_records_are_invalid);
    }

    env.clear();
    for(unsigned i = 0; i < numberOfRecords; i++)
    {
        if (!envIn[i].name || (strlen(envIn[i].name) == 0))
        {
            return OTEL_STATUS(environment_record_name_is_not_specified_or_empty);
        }
        std::string sName = envIn[i].name;

        if (!envIn[i].value) // we allow empty string for value
        {
            return OTEL_STATUS(environment_record_value_is_not_specified);
        }
        std::string sValue = envIn[i].value;

        env[sName] = sValue;
    }
    return OTEL_SUCCESS;
}

OTEL_SDK_STATUS_CODE PassedEnvinronmentReader::ReadMandatory(
	const std::string& varName, std::string& result)
{
    result.clear();

    std::map<std::string, std::string>::iterator found = env.find(varName);
    if(found == env.end())
    {
        LOG4CXX_ERROR(ApiUtils::apiLogger, boost::format("Environment variable %1% must be specified") % varName.c_str());
        return OTEL_STATUS(unspecified_environment_variable);
    }

    std::string value = (*found).second;
    if(value.empty())
    {
       LOG4CXX_ERROR(ApiUtils::apiLogger, boost::format("Environment variable %1% must be non-empty") % varName.c_str());
       return OTEL_STATUS(environment_variable_invalid_value);
    }

    result = value;

    return OTEL_SUCCESS;
}

OTEL_SDK_STATUS_CODE PassedEnvinronmentReader::ReadOptional(
	const std::string& varName, std::string& result)
{
    result.clear();

    std::map<std::string, std::string>::iterator found = env.find(varName);
    if(found == env.end())
    {
        LOG4CXX_TRACE(ApiUtils::apiLogger, boost::format("Environment variable %1% is not specified") % varName.c_str());
        return OTEL_SUCCESS;
    }

    std::string value = (*found).second;
    if(value.empty())
    {
        LOG4CXX_TRACE(ApiUtils::apiLogger, boost::format("Environment variable %1% is non-empty") % varName.c_str());
        return OTEL_SUCCESS;
    }

    result = value;

    return OTEL_SUCCESS;

}

}
}
