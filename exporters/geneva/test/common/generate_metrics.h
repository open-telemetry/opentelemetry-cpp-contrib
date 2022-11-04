#include "opentelemetry/sdk/metrics/data/metric_data.h"
#include "opentelemetry/sdk/metrics/export/metric_producer.h"
#include "opentelemetry/sdk/metrics/instruments.h"
#include "opentelemetry/sdk/resource/resource.h"

namespace {

// Instrument Scope
const std::string kInstrumentScopeName = "test_lib";
const std::string kInstrumentScopeVer = "1.5.0";

// Counter Instrument of double type
const std::string kCounterDoubleInstrumentName =
    "test_instrument_couter_double_name";
const std::string kCounterDoubleInstrumentDesc =
    "test_instrument_counter_double_desc";
const std::string kCounterDoubleInstrumentUnit =
    "test_instrument_conter_double_unit";
const double kCounterDoubleValue1 = 10.2;
const double kCounterDoubleValue2 = 20.2;
const std::string kCounterDoubleAttributeKey1 = "counter_double_key1";
const std::string kCounterDoubleAttributeValue1 = "counter_double_value1";
const std::string kCounterDoubleAttributeKey2 = "counter_double_key2";
const std::string kCounterDoubleAttributeValue2 = "counter_double_value2";
const std::string kCounterDoubleAttributeKey3 = "counter_double_key3";
const std::string kCounterDoubleAttributeValue3 = "counter_double_value3";
const uint16_t kCounterDoubleCountDimensions = 1;
const uint16_t kCounterDoubleEventId = 55;

static inline opentelemetry::sdk::metrics::ResourceMetrics
GenerateSumDataDoubleMetrics() {

  opentelemetry::sdk::metrics::SumPointData sum_point_data1{};
  sum_point_data1.value_ = kCounterDoubleValue1;
  opentelemetry::sdk::metrics::SumPointData sum_point_data2{};
  sum_point_data2.value_ = kCounterDoubleValue2;
  opentelemetry::sdk::metrics::ResourceMetrics data;
  auto resource = opentelemetry::sdk::resource::Resource::Create(
      opentelemetry::sdk::resource::ResourceAttributes{});
  data.resource_ = &resource;
  auto scope =
      opentelemetry::sdk::instrumentationscope::InstrumentationScope::Create(
          kInstrumentScopeName, kInstrumentScopeVer);
  opentelemetry::sdk::metrics::MetricData metric_data{
      opentelemetry::sdk::metrics::InstrumentDescriptor{
          kCounterDoubleInstrumentName, kCounterDoubleInstrumentDesc,
          kCounterDoubleInstrumentUnit,
          opentelemetry::sdk::metrics::InstrumentType::kCounter,
          opentelemetry::sdk::metrics::InstrumentValueType::kDouble},
      opentelemetry::sdk::metrics::AggregationTemporality::kDelta,
      opentelemetry::common::SystemTimestamp{std::chrono::system_clock::now()},
      opentelemetry::common::SystemTimestamp{std::chrono::system_clock::now()},
      std::vector<opentelemetry::sdk::metrics::PointDataAttributes>{
          {opentelemetry::sdk::metrics::PointAttributes{
               {kCounterDoubleAttributeKey1, kCounterDoubleAttributeValue1}},
           sum_point_data1},
          {opentelemetry::sdk::metrics::PointAttributes{
               {kCounterDoubleAttributeKey2, kCounterDoubleAttributeValue2},
               {kCounterDoubleAttributeKey3, kCounterDoubleAttributeValue3}},
           sum_point_data2}}};
  data.scope_metric_data_ =
      std::vector<opentelemetry::sdk::metrics::ScopeMetrics>{
          {scope.get(),
           std::vector<opentelemetry::sdk::metrics::MetricData>{metric_data}}};
  return data;
}

// Counter Instrument of long type
const std::string kCounterLongInstrumentName =
    "test_instrument_couter_long_name";
const std::string kCounterLongInstrumentDesc =
    "test_instrument_counter_long_desc";
const std::string kCounterLongInstrumentUnit =
    "test_instrument_conter_long_unit";
const long kCounterLongValue = 102;
const std::string kCounterLongAttributeKey1 = "counter_long_key1";
const std::string kCounterLongAttributeValue1 = "counter_long_value1";

const uint16_t kCounterLongCountDimensions = 1;
const uint16_t kCounterLongEventId = 50;

static inline opentelemetry::sdk::metrics::ResourceMetrics
GenerateSumDataLongMetrics() {

  opentelemetry::sdk::metrics::SumPointData sum_point_data{};
  sum_point_data.value_ = kCounterLongValue;
  opentelemetry::sdk::metrics::ResourceMetrics data;
  auto resource = opentelemetry::sdk::resource::Resource::Create(
      opentelemetry::sdk::resource::ResourceAttributes{});
  data.resource_ = &resource;
  auto scope =
      opentelemetry::sdk::instrumentationscope::InstrumentationScope::Create(
          kInstrumentScopeName, kInstrumentScopeVer);
  opentelemetry::sdk::metrics::MetricData metric_data{
      opentelemetry::sdk::metrics::InstrumentDescriptor{
          kCounterLongInstrumentName, kCounterLongInstrumentDesc,
          kCounterLongInstrumentUnit,
          opentelemetry::sdk::metrics::InstrumentType::kCounter,
          opentelemetry::sdk::metrics::InstrumentValueType::kLong},
      opentelemetry::sdk::metrics::AggregationTemporality::kDelta,
      opentelemetry::common::SystemTimestamp{std::chrono::system_clock::now()},
      opentelemetry::common::SystemTimestamp{std::chrono::system_clock::now()},
      std::vector<opentelemetry::sdk::metrics::PointDataAttributes>{
          {opentelemetry::sdk::metrics::PointAttributes{
               {kCounterLongAttributeKey1, kCounterLongAttributeValue1}},
           sum_point_data}}};
  data.scope_metric_data_ =
      std::vector<opentelemetry::sdk::metrics::ScopeMetrics>{
          {scope.get(),
           std::vector<opentelemetry::sdk::metrics::MetricData>{metric_data}}};
  return data;
}

// Histogram Instrument of type long
const std::string kHistogramLongInstrumentName =
    "test_instrument_histogram_long_name";
const std::string kHistogramLongInstrumentDesc =
    "test_instrument_histogram_long_desc";
const std::string kHistogramLongInstrumentUnit =
    "test_instrument_histogram_long_unit";

const long kHistogramLongSum = 4024l;
const long kHistogramLongMin = 3l;
const long kHistogramLongMax = 1004l;
const size_t kHistogramLongCount = 10l;
const uint16_t kHistogramLongBucketSize = 10;
const uint16_t kHistogramLongNonEmptyBucketSize = 8;
const std::vector<uint64_t> kHistogramLongCounts = {1, 2, 1, 0, 0,
                                                    1, 2, 3, 4, 1};
const std::list<double> kHistogramLongBoundaries = std::list<double>{
    0.0, 5.0, 10.0, 25.0, 50.0, 75.0, 100.0, 250.0, 500.0, 1000.0};

const std::string kHistogramLongAttributeKey1 = "histogram_long_key1";
const std::string kHistogramLongAttributeValue1 = "histogram_long_value1";

const uint16_t kHistogramLongCountDimensions = 1;
const uint16_t kHistogramLongEventId = 56;

static inline opentelemetry::sdk::metrics::ResourceMetrics
GenerateHistogramDataLongMetrics() {

  opentelemetry::sdk::metrics::HistogramPointData histogram_point_data{};
  histogram_point_data.sum_ = kHistogramLongSum;
  histogram_point_data.boundaries_ = kHistogramLongBoundaries;
  histogram_point_data.count_ = kHistogramLongCount;
  histogram_point_data.min_ = kHistogramLongMin;
  histogram_point_data.max_ = kHistogramLongMax;
  histogram_point_data.counts_ = kHistogramLongCounts;

  opentelemetry::sdk::metrics::ResourceMetrics data;
  auto resource = opentelemetry::sdk::resource::Resource::Create(
      opentelemetry::sdk::resource::ResourceAttributes{});
  data.resource_ = &resource;
  auto scope =
      opentelemetry::sdk::instrumentationscope::InstrumentationScope::Create(
          kInstrumentScopeName, kInstrumentScopeVer);
  opentelemetry::sdk::metrics::MetricData metric_data{
      opentelemetry::sdk::metrics::InstrumentDescriptor{
          kHistogramLongInstrumentName, kHistogramLongInstrumentDesc,
          kHistogramLongInstrumentUnit,
          opentelemetry::sdk::metrics::InstrumentType::kHistogram,
          opentelemetry::sdk::metrics::InstrumentValueType::kLong},
      opentelemetry::sdk::metrics::AggregationTemporality::kDelta,
      opentelemetry::common::SystemTimestamp{std::chrono::system_clock::now()},
      opentelemetry::common::SystemTimestamp{std::chrono::system_clock::now()},
      std::vector<opentelemetry::sdk::metrics::PointDataAttributes>{
          {opentelemetry::sdk::metrics::PointAttributes{
               {kHistogramLongAttributeKey1, kHistogramLongAttributeValue1}},
           histogram_point_data}}};
  data.scope_metric_data_ =
      std::vector<opentelemetry::sdk::metrics::ScopeMetrics>{
          {scope.get(),
           std::vector<opentelemetry::sdk::metrics::MetricData>{metric_data}}};
  return data;
}

} // namespace