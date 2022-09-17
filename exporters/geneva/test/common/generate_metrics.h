#  include "opentelemetry/sdk/metrics/data/metric_data.h"
#  include "opentelemetry/sdk/metrics/instruments.h"
#  include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/sdk/metrics/export/metric_producer.h"

namespace {


// Instrument Scope
const std::string kInstrumentScopeName = "test_lib";
const std::string kInstrumentScopeVer = "1.5.0";

// Counter Instrument of double type
const std::string kCounterDoubleInstrumentName = "test_instrument_name";
const std::string kCounterDoubleInstrumentDesc = "test_instrument_desc";
const std::string kCounterDoubleInstrumentUnit = "test_instrument_unit";
const double kCounterDoubleValue1 = 10.0;
const double kCounterDoubleValue2 = 20.0;
const std::string kCounterDoubleAttributeKey1 = "key1";
const std::string kCounterDoubleAttributeValue1 = "value1";
const std::string kCounterDoubleAttributeKey2 = "key2";
const std::string kCounterDoubleAttributeValue2 = "value2";
const uint16_t kCounterDoubleCountDimensions = 1;
const uint16_t kCounterDoubleEventId = 55;

static inline opentelemetry::sdk::metrics::ResourceMetrics GenerateSumDataMetrics() {

    opentelemetry::sdk::metrics::SumPointData sum_point_data{};
    sum_point_data.value_ = kCounterDoubleValue1;
    opentelemetry::sdk::metrics::SumPointData sum_point_data2{};
    sum_point_data2.value_ = kCounterDoubleValue2;
    opentelemetry::sdk::metrics::ResourceMetrics data;
    auto resource = opentelemetry::sdk::resource::Resource::Create(
        opentelemetry::sdk::resource::ResourceAttributes{});
    data.resource_ = &resource;
    auto scope     = opentelemetry::sdk::instrumentationscope::InstrumentationScope::Create(
        kInstrumentScopeName, kInstrumentScopeVer);
    opentelemetry::sdk::metrics::MetricData metric_data{
        opentelemetry::sdk::metrics::InstrumentDescriptor{
            kCounterDoubleInstrumentName, kCounterDoubleInstrumentDesc, kCounterDoubleInstrumentUnit,
            opentelemetry::sdk::metrics::InstrumentType::kCounter,
            opentelemetry::sdk::metrics::InstrumentValueType::kDouble},
        opentelemetry::sdk::metrics::AggregationTemporality::kDelta,
        opentelemetry::common::SystemTimestamp{}, opentelemetry::common::SystemTimestamp{},
        std::vector<opentelemetry::sdk::metrics::PointDataAttributes>{
            {opentelemetry::sdk::metrics::PointAttributes{{kCounterDoubleAttributeKey1, kCounterDoubleAttributeValue1}}, sum_point_data}}};
           // {opentelemetry::sdk::metrics::PointAttributes{{"a2", "b2"}}, sum_point_data2}}*/
    data.scope_metric_data_ = std::vector<opentelemetry::sdk::metrics::ScopeMetrics>{{scope.get(), std::vector<opentelemetry::sdk::metrics::MetricData>{metric_data}}};
    return data;
}
}