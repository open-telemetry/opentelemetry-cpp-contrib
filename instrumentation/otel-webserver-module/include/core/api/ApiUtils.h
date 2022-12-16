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

#ifndef APIUTILS_H
#define APIUTILS_H

#include <string>
#include <map>
#include <api/OpentelemetrySdk.h>
#include "AgentLogger.h"
#include <api/Interface.h>

namespace otel {
namespace core {

class TenantConfig;

/*class RealEnvinronmentReader : public IEnvReader
{
public:
    virtual OTEL_SDK_STATUS_CODE ReadMandatory(const std::string& varName, std::string& result);
    virtual OTEL_SDK_STATUS_CODE ReadOptional(const std::string& varName, std::string& result);
};*/

class PassedEnvinronmentReader : public IEnvReader
{
public:
    OTEL_SDK_STATUS_CODE Init(OTEL_SDK_ENV_RECORD* env, unsigned numberOfRecords);

    virtual OTEL_SDK_STATUS_CODE ReadMandatory(const std::string& varName, std::string& result);
    virtual OTEL_SDK_STATUS_CODE ReadOptional(const std::string& varName, std::string& result);

private:
    std::map<std::string, std::string> env;
};

class ApiUtils : public IApiUtils
{
public:
    static AgentLogger apiLogger;
    static AgentLogger apiUserLogger;

    static boost::filesystem::path getSDKInstallPath();
    static void cleanup();
    /*
    *   TODO: Following functions will be implemented while doing agent init and term
    */
    OTEL_SDK_STATUS_CODE init_boilerplate() override; // initializes agentLogging


    OTEL_SDK_STATUS_CODE ReadFromPassedSettings(
            OTEL_SDK_ENV_RECORD* env,
            unsigned numberOfRecords,
            TenantConfig& tenantConfig,
            SpanNamer& spanNamer) override;
    //OTEL_SDK_STATUS_CODE ReadFromEnvinronment(TenantConfig&) override;

protected:
    OTEL_SDK_STATUS_CODE ReadSettingsFromReader(
        IEnvReader& reader, TenantConfig&, SpanNamer&) override;
    /*OTEL_SDK_STATUS_CODE ReadMandatoryFromReader(
        IEnvReader& reader, const std::string& varName, unsigned short& result) override;*/
    OTEL_SDK_STATUS_CODE ReadOptionalFromReader(
        IEnvReader& reader, const std::string& varName, bool& result) override;
    OTEL_SDK_STATUS_CODE ReadOptionalFromReader(
        IEnvReader& reader, const std::string& varName, unsigned int& result) override;
};

}
}

#endif  /* APIUTILS_H */
