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
#ifndef __MOCKOPENTELEMETRY_H
#define __MOCKOPENTELEMETRY_H

#include <gmock/gmock.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/span_context_kv_iterable.h>
#include <opentelemetry/common/key_value_iterable.h>
#include <opentelemetry/common/attribute_value.h>
#include <opentelemetry/common/timestamp.h>
#include <opentelemetry/nostd/string_view.h>
#include <opentelemetry/trace/span_context.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>

using namespace opentelemetry;

class MockTracer : public opentelemetry::trace::Tracer
{
public:
	virtual nostd::shared_ptr<trace::Span> StartSpan(nostd::string_view name,
	    const common::KeyValueIterable &attributes,
	    const trace::SpanContextKeyValueIterable &links,
	    const trace::StartSpanOptions &options = {}) noexcept
	{
		attributes.ForEachKeyValue([&](
			nostd::string_view key, common::AttributeValue value) noexcept {
    		VerifyKeyValueIterable(key, value);
    		return true;
    	});
    	return StartSpanInternal(name, options.kind);
	}

	MOCK_METHOD(void, VerifyKeyValueIterable, (nostd::string_view name,
		common::AttributeValue value));

	MOCK_METHOD(nostd::shared_ptr<trace::Span>, StartSpanInternal,
		(nostd::string_view name, trace::SpanKind kind));

	MOCK_METHOD(void, ForceFlushWithMicroseconds, (uint64_t timeout), (noexcept));
	MOCK_METHOD(void, CloseWithMicroseconds, (uint64_t timeout), (noexcept));
};

class MockSpan : public opentelemetry::trace::Span
{
public:
	virtual void AddEvent(nostd::string_view name,
	                      common::SystemTimestamp timestamp,
                        const common::KeyValueIterable &attributes) noexcept {
		AddEvent(name, timestamp);
		attributes.ForEachKeyValue([&](
			nostd::string_view key, common::AttributeValue value) noexcept {
    		VerifyKeyValueIterable(key, value);
    		return true;
  		});
	}

	MOCK_METHOD(void, VerifyKeyValueIterable, (nostd::string_view name,
		common::AttributeValue value));

	MOCK_METHOD(void, SetAttribute,
		(nostd::string_view key, const common::AttributeValue &value), (noexcept));
	MOCK_METHOD(void, AddEvent, (nostd::string_view name), (noexcept));
	MOCK_METHOD(void, AddEvent, (nostd::string_view name, common::SystemTimestamp timestamp), (noexcept));
	MOCK_METHOD(void, SetStatus, (trace::StatusCode code, nostd::string_view description), (noexcept));
	MOCK_METHOD(void, UpdateName, (nostd::string_view name), (noexcept));
	MOCK_METHOD(void, End, (const trace::EndSpanOptions &options), (noexcept));
	MOCK_METHOD(trace::SpanContext,  GetContext, (), (const, noexcept));
	MOCK_METHOD(bool, IsRecording, (), (const, noexcept));
};

class MockPropagator : public context::propagation::TextMapPropagator
{
public:

  	virtual context::Context Extract(
        const context::propagation::TextMapCarrier &carrier,
        context::Context &context) noexcept override {
  		ExtractImpl(carrier);
  		return context;
  	}

  	virtual void Inject(
        context::propagation::TextMapCarrier &carrier,
        const context::Context &context) noexcept override {
  		InjectImpl(carrier);
  	}
	
  	MOCK_METHOD(void, ExtractImpl, (const context::propagation::TextMapCarrier &carrier));
  	MOCK_METHOD(void, InjectImpl, (context::propagation::TextMapCarrier &carrier));
	MOCK_METHOD(bool, Fields, (nostd::function_ref<bool(nostd::string_view)> callback), 
		(const, noexcept, override));
};

#endif
