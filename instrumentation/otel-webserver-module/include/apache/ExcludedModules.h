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

#ifndef EXCLUDEDMODULES_H
#define EXCLUDEDMODULES_H

#include <unordered_set>
#include <string>
#include <vector>
#include <httpd.h>
#include "HookContainer.h"

// hook_get_t is a pointer to a function that takes void as an argument and
// returns a pointer to an apr_array_header_t.
typedef apr_array_header_t *(
*hook_get_t)      (void);

class ExcludedModules
{
public:
    static const std::unordered_set<std::string> excludedAlways; // modules that can never be instumented.
    static std::unordered_set<std::string> userSpecified;        // modules requested by user
    static const std::unordered_set<std::string> excluded24;     // Apache 2.4 excluded module list
    static const std::unordered_set<std::string> excluded22;     // Apache 2.2 excluded module list

    // Find the modules that we want to instrument for a stage.
    // Include modules that are specified by the user and modules NOT in the exclude lists.
    static void findHookPoints(
            std::vector<HookInfo> &hooks_found,
            hook_get_t getHooks,
            const std::string& stage);
    static void getUserSpecifiedModules(const char* modules);
};

#endif
