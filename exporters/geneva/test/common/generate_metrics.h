#  include "opentelemetry/sdk/metrics/data/metric_data.h"
#  include "opentelemetry/sdk/metrics/instruments.h"
#  include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/sdk/metrics/export/metric_producer.h"


static inline opentelemetry::sdk::metrics::ResourceMetrics GenerateSumDataMetrics() {

    opentelemetry::sdk::metrics::SumPointData sum_point_data{};
    sum_point_data.value_ = 10.0;
    opentelemetry::sdk::metrics::SumPointData sum_point_data2{};
    sum_point_data2.value_ = 20.0;
    opentelemetry::sdk::metrics::ResourceMetrics data;
    auto resource = opentelemetry::sdk::resource::Resource::Create(
        opentelemetry::sdk::resource::ResourceAttributes{});
    data.resource_ = &resource;
    auto scope     = opentelemetry::sdk::instrumentationscope::InstrumentationScope::Create(
        "library_name", "1.5.0");
    opentelemetry::sdk::metrics::MetricData metric_data{
        opentelemetry::sdk::metrics::InstrumentDescriptor{
            "metrics_library_name", "metrics_description", "metrics_unit",
            opentelemetry::sdk::metrics::InstrumentType::kCounter,
            opentelemetry::sdk::metrics::InstrumentValueType::kDouble},
        opentelemetry::sdk::metrics::AggregationTemporality::kDelta,
        opentelemetry::common::SystemTimestamp{}, opentelemetry::common::SystemTimestamp{std::chrono::system_clock::now()},
        std::vector<opentelemetry::sdk::metrics::PointDataAttributes>{
            {opentelemetry::sdk::metrics::PointAttributes{{"a1", "b1"}}, sum_point_data} /*,
            {opentelemetry::sdk::metrics::PointAttributes{{"a2", "b2"}}, sum_point_data2}*/}};
    data.scope_metric_data_ = std::vector<opentelemetry::sdk::metrics::ScopeMetrics>{
        {scope.get(), std::vector<opentelemetry::sdk::metrics::MetricData>{metric_data}}};
    return data;
}