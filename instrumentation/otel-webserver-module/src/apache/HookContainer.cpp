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
#include "HookContainer.h"

HookInfo::HookInfo(const std::string& s, const std::string& m, int i) : stage(s), module(m), order(i)
{
    moduleList[0] = module.c_str();
    moduleList[1] = NULL;
}

HookContainer& HookContainer::getInstance()
{
    static HookContainer instance;
    return instance;
}

void HookContainer::addHook(
        otel_endpoint_indexes index,
        const std::string& stage,
        const std::string& module,
        int order)
{
    hooks.insert(std::pair<int, HookInfo>(index, HookInfo(stage, module, order)));
}

const std::string& HookContainer::getStage(otel_endpoint_indexes index) const
{
    static std::string blank;
    auto it = hooks.find(index);
    return (it == hooks.end() ? blank : it->second.stage);
}

const std::string& HookContainer::getModule(otel_endpoint_indexes index) const
{
    static std::string blank;
    auto it = hooks.find(index);
    return (it == hooks.end() ? blank : it->second.module);
}

const char* const* HookContainer::getModuleList(otel_endpoint_indexes index) const
{
    auto it = hooks.find(index);
    return (it == hooks.end() ? NULL : it->second.moduleList);
}

void HookContainer::traceHooks(request_rec* r) const
{
    std::string modules;

    auto separator = "Modules instumented:";
    for (auto ii = hooks.begin(); ii != hooks.end(); ++ii)
    {
        modules += separator;
        separator = ",";
        modules += ii->second.stage;
        modules += ":";
        modules += ii->second.module;
    }
}
