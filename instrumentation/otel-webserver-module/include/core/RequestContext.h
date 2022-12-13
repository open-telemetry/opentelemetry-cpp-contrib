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

#ifndef __REQUESTCONTEXT_H
#define __REQUESTCONTEXT_H

#include <memory>
#include <stack>

namespace otel {
namespace core {

namespace sdkwrapper {

class IScopedSpan;

}

/**
    Class that keeps track of request specific information.
    An instance of this class is created at the beginning of each
    call of a request and destroyed at the end of each call.
 */
class RequestContext
{
public:

    // TODO : this constructor is used only in testing. Revisit this later to remove
    RequestContext() = default;

    RequestContext(std::shared_ptr<sdkwrapper::IScopedSpan> rootSpan)
    : m_rootSpan(rootSpan) {}

    ~RequestContext() = default;

    std::shared_ptr<sdkwrapper::IScopedSpan> rootSpan() const { return m_rootSpan; }

    void addInteraction(std::shared_ptr<sdkwrapper::IScopedSpan> interaction) {
        m_interactions.push(interaction);
    }

    void removeInteraction() {
        // Interactions are LIFO. So, we pop the last interaction.
        m_interactions.pop();
    }

    bool hasActiveInteraction() const {
        return !m_interactions.empty();
    }

    std::shared_ptr<sdkwrapper::IScopedSpan> lastActiveInteraction() {
        return m_interactions.top();
    }

    void setContextName(const std::string& contextName) {
        m_contextName = contextName;
    }

    std::string getContextName() const {
        return m_contextName;
    }

private:
    std::shared_ptr<sdkwrapper::IScopedSpan> m_rootSpan;
    std::string m_contextName;

    /**
      List of all the active interactions associated with this request
    */
    std::stack<std::shared_ptr<sdkwrapper::IScopedSpan> > m_interactions;

};

} // otel
} // core

#endif
