// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#if !defined(__CYGWIN__) && defined(_WIN32)
#  include <direct.h>
#  include <io.h>
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#include "opentelemetry/exporters/prometheus/file_exporter.h"
#include "opentelemetry/exporters/prometheus/file_exporter_factory.h"
#include "opentelemetry/exporters/prometheus/file_exporter_options.h"
#include "opentelemetry/sdk/instrumentationscope/instrumentation_scope.h"
#include "opentelemetry/sdk/metrics/export/metric_producer.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "prometheus_test_helper.h"

using opentelemetry::exporter::metrics::PrometheusFileExporter;
using opentelemetry::exporter::metrics::PrometheusFileExporterFactory;
using opentelemetry::exporter::metrics::PrometheusFileExporterOptions;
using opentelemetry::sdk::common::ExportResult;

namespace
{

bool FileExists(const std::string &path)
{
#if !defined(__CYGWIN__) && defined(_WIN32)
  return 0 == _access(path.c_str(), 0);
#else
  return 0 == access(path.c_str(), F_OK);
#endif
}

std::string ReadFileContent(const std::string &path)
{
  std::ifstream file(path.c_str(), std::ios::binary | std::ios::in);
  if (!file.is_open())
  {
    return std::string();
  }
  return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void RemoveDirectoryIfEmpty(const std::string &path)
{
#if !defined(__CYGWIN__) && defined(_WIN32)
  _rmdir(path.c_str());
#else
  rmdir(path.c_str());
#endif
}

// Single level directory creation, enough for the flat test directories
void CreateDirectoryIfMissing(const std::string &path)
{
#if !defined(__CYGWIN__) && defined(_WIN32)
  _mkdir(path.c_str());
#else
  mkdir(path.c_str(), 0755);
#endif
}

/**
 * Removes the given files and directories before the test starts (to clean up
 * leftovers of previous runs) and after the test ends.
 *
 * It must be declared before the exporter, so that the exporter and its open
 * log files are destroyed first.
 */
class TestFileCleaner
{
public:
  TestFileCleaner(std::vector<std::string> file_paths, std::vector<std::string> dir_paths)
      : file_paths_(std::move(file_paths)), dir_paths_(std::move(dir_paths))
  {
    Cleanup();
  }

  ~TestFileCleaner() { Cleanup(); }

  void Cleanup()
  {
    for (const auto &path : file_paths_)
    {
      std::remove(path.c_str());
    }
    for (const auto &path : dir_paths_)
    {
      RemoveDirectoryIfEmpty(path);
    }
  }

private:
  std::vector<std::string> file_paths_;
  std::vector<std::string> dir_paths_;
};

std::tm GetLocalTimeValue()
{
  std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
#if defined(_MSC_VER) && _MSC_VER >= 1300
  std::tm ret;
  localtime_s(&ret, &now);
#elif (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || defined(__STDC_LIB_EXT1__)
  std::tm ret;
  localtime_s(&now, &ret);
#elif defined(_XOPEN_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || \
    defined(_POSIX_SOURCE)
  std::tm ret;
  localtime_r(&now, &ret);
#else
  std::tm ret = *localtime(&now);
#endif
  return ret;
}

std::string GetLocalDateStamp()
{
  std::tm tm_value = GetLocalTimeValue();
  char buffer[16]  = {0};
  std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", tm_value.tm_year + 1900,
                tm_value.tm_mon + 1, tm_value.tm_mday);
  return buffer;
}

/**
 * Keeps the instrumentation scope alive while the ResourceMetrics created from
 * it is used, the metrics data stores the scope by raw pointer.
 */
class ScopedMetricData
{
public:
  explicit ScopedMetricData(const std::string &scope_name)
      : scope_(opentelemetry::sdk::instrumentationscope::InstrumentationScope::Create(scope_name,
                                                                                     "1.9.1"))
  {}

  metric_sdk::ResourceMetrics Sum() const { return CreateSumPointData(scope_.get()); }

  metric_sdk::ResourceMetrics Histogram() const { return CreateHistogramPointData(scope_.get()); }

  metric_sdk::ResourceMetrics LastValue() const { return CreateLastValuePointData(scope_.get()); }

  metric_sdk::ResourceMetrics Drop() const { return CreateDropPointData(scope_.get()); }

private:
  std::unique_ptr<opentelemetry::sdk::instrumentationscope::InstrumentationScope> scope_;
};

PrometheusFileExporterOptions MakeOptions(const std::string &directory,
                                          const std::string &file_name)
{
  PrometheusFileExporterOptions options;
  options.file_pattern = directory + "/" + file_name;
  // The default alias pattern is date based and would create files unknown to
  // the test cleanup.
  options.alias_pattern.clear();
  // Flush inline after every export and do not spawn the background flush
  // thread, so that the tests are deterministic.
  options.flush_interval = std::chrono::microseconds{0};
  options.flush_count    = 1;
  return options;
}

}  // namespace

/**
 * When a PrometheusFileExporter is initialized,
 * isShutdown should be false.
 */
TEST(PrometheusFileExporter, InitializeConstructorIsNotShutdown)
{
  PrometheusFileExporterOptions options;
  PrometheusFileExporter exporter(options);

  // Asserts that the exporter is not shutdown.
  ASSERT_TRUE(!exporter.IsShutdown());
}

/**
 * The shutdown() function should set the isShutdown field to true.
 */
TEST(PrometheusFileExporter, ShutdownSetsIsShutdownToTrue)
{
  PrometheusFileExporterOptions options;
  PrometheusFileExporter exporter(options);

  // exporter should not be shutdown by default
  ASSERT_TRUE(!exporter.IsShutdown());

  ASSERT_TRUE(exporter.Shutdown(std::chrono::microseconds{1000}));

  // the exporter should be shutdown
  ASSERT_TRUE(exporter.IsShutdown());

  // shutdown function should be idempotent
  ASSERT_TRUE(exporter.Shutdown(std::chrono::microseconds{1000}));
  ASSERT_TRUE(exporter.IsShutdown());
}

/**
 * The Prometheus file exporter only supports cumulative aggregation
 * temporality for all instrument types.
 */
TEST(PrometheusFileExporter, GetAggregationTemporalityIsCumulative)
{
  PrometheusFileExporterOptions options;
  PrometheusFileExporter exporter(options);

  const metric_sdk::InstrumentType instrument_types[] = {
      metric_sdk::InstrumentType::kCounter,
      metric_sdk::InstrumentType::kHistogram,
      metric_sdk::InstrumentType::kGauge,
      metric_sdk::InstrumentType::kUpDownCounter,
      metric_sdk::InstrumentType::kObservableCounter,
      metric_sdk::InstrumentType::kObservableGauge,
      metric_sdk::InstrumentType::kObservableUpDownCounter,
  };

  for (auto instrument_type : instrument_types)
  {
    ASSERT_EQ(exporter.GetAggregationTemporality(instrument_type),
              metric_sdk::AggregationTemporality::kCumulative);
  }
}

/**
 * The Export() function should return kSuccess = 0
 * when data is exported successfully, and the metrics should be
 * written to the file expanded from the file pattern (%N is the
 * 0-based rotate index).
 */
TEST(PrometheusFileExporter, ExportSuccessfully)
{
  const std::string directory = "prometheus_file_exporter_test_export_successfully";
  const std::string log_file  = directory + "/metrics.0.log";
  TestFileCleaner cleaner({log_file}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "metrics.%N.log");
  PrometheusFileExporter exporter(options);

  ScopedMetricData data("export_successfully_scope");
  auto res = exporter.Export(data.Sum());

  // result should be kSuccess = 0
  ExportResult code = ExportResult::kSuccess;
  ASSERT_EQ(res, code);

  const std::string content = ReadFileContent(log_file);
  ASSERT_FALSE(content.empty());
  ASSERT_NE(content.find("# TYPE"), std::string::npos);
  ASSERT_NE(content.find("library_name"), std::string::npos);
  // both point attribute sets are exported
  ASSERT_NE(content.find("a1"), std::string::npos);
  ASSERT_NE(content.find("a2"), std::string::npos);
  // otel scope labels are populated by default
  ASSERT_NE(content.find("otel_scope_name"), std::string::npos);
  ASSERT_NE(content.find("export_successfully_scope"), std::string::npos);
}

/**
 * If the exporter is shutdown, it cannot process any more export requests
 * and returns kFailure = 1, without touching any file.
 */
TEST(PrometheusFileExporter, ExporterIsShutdown)
{
  const std::string directory = "prometheus_file_exporter_test_shutdown";
  const std::string log_file  = directory + "/metrics.0.log";
  TestFileCleaner cleaner({log_file}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "metrics.%N.log");
  PrometheusFileExporter exporter(options);

  ASSERT_TRUE(exporter.Shutdown(std::chrono::microseconds{1000}));

  ScopedMetricData data("shutdown_scope");
  auto res = exporter.Export(data.Sum());

  // result code should be kFailure = 1
  ExportResult code = ExportResult::kFailure;
  ASSERT_EQ(res, code);
  ASSERT_FALSE(FileExists(log_file));
}

/**
 * The Export() function accepts an empty collection of records and
 * creates an empty log file for it.
 */
TEST(PrometheusFileExporter, ExportEmptyRecordCollection)
{
  const std::string directory = "prometheus_file_exporter_test_empty";
  const std::string log_file  = directory + "/metrics.0.log";
  TestFileCleaner cleaner({log_file}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "metrics.%N.log");
  PrometheusFileExporter exporter(options);

  metric_sdk::ResourceMetrics data;
  data.resource_ = &GetEmptyResource();

  auto res = exporter.Export(data);

  ExportResult code = ExportResult::kSuccess;
  ASSERT_EQ(res, code);
  ASSERT_TRUE(FileExists(log_file));
}

/**
 * All point data types supported by the Prometheus text format are
 * serialized into the log file.
 */
TEST(PrometheusFileExporter, ExportDifferentPointDataTypes)
{
  const std::string directory = "prometheus_file_exporter_test_point_data_types";
  const std::string log_file  = directory + "/metrics.0.log";
  TestFileCleaner cleaner({log_file}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "metrics.%N.log");
  PrometheusFileExporter exporter(options);

  ScopedMetricData data("point_data_types_scope");
  ASSERT_EQ(exporter.Export(data.Sum()), ExportResult::kSuccess);
  ASSERT_EQ(exporter.Export(data.Histogram()), ExportResult::kSuccess);
  ASSERT_EQ(exporter.Export(data.LastValue()), ExportResult::kSuccess);
  ASSERT_EQ(exporter.Export(data.Drop()), ExportResult::kSuccess);

  const std::string content = ReadFileContent(log_file);
  ASSERT_FALSE(content.empty());
  ASSERT_NE(content.find("library_name"), std::string::npos);
  // histograms are exported with their buckets
  ASSERT_NE(content.find("_bucket"), std::string::npos);
  ASSERT_NE(content.find("otel_scope_name"), std::string::npos);
}

/**
 * When the size of the current log file reaches options.file_size, the next
 * export rotates to the next file of options.file_pattern. With
 * rotate_size = 3 the 4th export wraps around to the first file and
 * truncates its previous content.
 */
TEST(PrometheusFileExporter, RotatesFilesWhenSizeLimitIsReached)
{
  const std::string directory = "prometheus_file_exporter_test_rotation";
  const std::vector<std::string> log_files = {directory + "/rotation.0.log",
                                              directory + "/rotation.1.log",
                                              directory + "/rotation.2.log",
                                              directory + "/rotation.3.log"};
  TestFileCleaner cleaner(log_files, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "rotation.%N.log");
  options.file_size   = 1;  // any written data triggers the rotation on the next export
  options.rotate_size = 3;
  PrometheusFileExporter exporter(options);

  for (int i = 1; i <= 4; ++i)
  {
    ScopedMetricData data("rotation_scope_" + std::to_string(i));
    ASSERT_EQ(exporter.Export(data.Sum()), ExportResult::kSuccess);
  }

  // The exports went to rotation.0/1/2.log and then wrapped around
  for (std::size_t i = 0; i < 3; ++i)
  {
    ASSERT_TRUE(FileExists(log_files[i])) << log_files[i];
  }
  ASSERT_FALSE(FileExists(log_files[3]));

  ASSERT_NE(ReadFileContent(log_files[1]).find("rotation_scope_2"), std::string::npos);
  ASSERT_NE(ReadFileContent(log_files[2]).find("rotation_scope_3"), std::string::npos);

  // the first file is truncated by the wrap-around export
  const std::string first_file_content = ReadFileContent(log_files[0]);
  ASSERT_NE(first_file_content.find("rotation_scope_4"), std::string::npos);
  ASSERT_EQ(first_file_content.find("rotation_scope_1"), std::string::npos);
}

/**
 * On the first export, the exporter starts from the first rotate file
 * that has not reached options.file_size yet.
 */
TEST(PrometheusFileExporter, InitializationSkipsFullRotateFiles)
{
  const std::string directory = "prometheus_file_exporter_test_resume";
  const std::vector<std::string> log_files = {directory + "/resume.0.log",
                                              directory + "/resume.1.log",
                                              directory + "/resume.2.log"};
  TestFileCleaner cleaner(log_files, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "resume.%N.log");
  options.file_size   = 1;
  options.rotate_size = 3;

  // Simulate a full first rotate file left by a previous run
  CreateDirectoryIfMissing(directory);
  {
    std::ofstream file(log_files[0], std::ios::binary | std::ios::out);
    file << "previous run content";
  }

  PrometheusFileExporter exporter(options);

  ScopedMetricData data("resume_scope");
  ASSERT_EQ(exporter.Export(data.Sum()), ExportResult::kSuccess);

  // the first export appends to the second rotate file
  ASSERT_NE(ReadFileContent(log_files[1]).find("resume_scope"), std::string::npos);
  // the full file is not touched
  ASSERT_EQ(ReadFileContent(log_files[0]), "previous run content");
}

/**
 * When alias_pattern is set, a hard link with the expanded alias name is
 * created for the current log file (%n is the 1-based rotate index).
 */
TEST(PrometheusFileExporter, AliasPatternPointsToCurrentLogFile)
{
  const std::string directory  = "prometheus_file_exporter_test_alias";
  const std::string log_file   = directory + "/aliased.1.log";
  const std::string alias_file = directory + "/aliased.current.log";
  TestFileCleaner cleaner({log_file, alias_file}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "aliased.%n.log");
  options.alias_pattern = alias_file;
  PrometheusFileExporter exporter(options);

  ScopedMetricData data("alias_scope");
  ASSERT_EQ(exporter.Export(data.Sum()), ExportResult::kSuccess);

  ASSERT_TRUE(FileExists(log_file));
  ASSERT_TRUE(FileExists(alias_file));
  const std::string content = ReadFileContent(log_file);
  ASSERT_FALSE(content.empty());
  ASSERT_EQ(ReadFileContent(alias_file), content);
}

/**
 * Datetime placeholders in file_pattern are expanded with the current
 * local time.
 */
TEST(PrometheusFileExporter, FilePatternExpandsDatetimePlaceholders)
{
  const std::string directory = "prometheus_file_exporter_test_datetime";
  const std::string date_before_test = GetLocalDateStamp();
  TestFileCleaner cleaner({directory + "/" + date_before_test + ".metrics.log"}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "%Y-%m-%d.metrics.log");
  PrometheusFileExporter exporter(options);

  ScopedMetricData data("datetime_scope");
  ASSERT_EQ(exporter.Export(data.Sum()), ExportResult::kSuccess);

  // The date may change between the export and the check, accept both
  const std::string date_after_test = GetLocalDateStamp();
  std::string content = ReadFileContent(directory + "/" + date_before_test + ".metrics.log");
  if (content.empty() && date_after_test != date_before_test)
  {
    content = ReadFileContent(directory + "/" + date_after_test + ".metrics.log");
  }
  ASSERT_NE(content.find("datetime_scope"), std::string::npos);
}

/**
 * When without_otel_scope is set, the otel scope labels are not written.
 */
TEST(PrometheusFileExporter, WithoutOtelScopeOmitsScopeLabels)
{
  const std::string directory = "prometheus_file_exporter_test_without_scope";
  const std::string log_file  = directory + "/metrics.0.log";
  TestFileCleaner cleaner({log_file}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "metrics.%N.log");
  options.without_otel_scope = true;
  PrometheusFileExporter exporter(options);

  ScopedMetricData data("without_scope_test_scope");
  ASSERT_EQ(exporter.Export(data.Sum()), ExportResult::kSuccess);

  const std::string content = ReadFileContent(log_file);
  ASSERT_NE(content.find("library_name"), std::string::npos);
  ASSERT_EQ(content.find("otel_scope_name"), std::string::npos);
  ASSERT_EQ(content.find("without_scope_test_scope"), std::string::npos);
}

/**
 * ForceFlush() returns true when all exported metrics are already flushed
 * by flush_count.
 */
TEST(PrometheusFileExporter, ForceFlushWithInlineData)
{
  const std::string directory = "prometheus_file_exporter_test_force_flush";
  const std::string log_file  = directory + "/metrics.0.log";
  TestFileCleaner cleaner({log_file}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "metrics.%N.log");
  PrometheusFileExporter exporter(options);

  ScopedMetricData data("force_flush_scope");
  ASSERT_EQ(exporter.Export(data.Sum()), ExportResult::kSuccess);

  ASSERT_TRUE(exporter.ForceFlush(std::chrono::microseconds{0}));
  ASSERT_FALSE(ReadFileContent(log_file).empty());
}

/**
 * ForceFlush() waits for the background flush thread when the metrics
 * are not flushed inline.
 */
TEST(PrometheusFileExporter, ForceFlushWaitsForBackgroundFlush)
{
  const std::string directory = "prometheus_file_exporter_test_background_flush";
  const std::string log_file  = directory + "/metrics.0.log";
  TestFileCleaner cleaner({log_file}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "metrics.%N.log");
  options.flush_interval = std::chrono::microseconds{10 * 1000};  // 10ms
  options.flush_count    = 0;  // only the background flush thread flushes
  PrometheusFileExporter exporter(options);

  ScopedMetricData data("background_flush_scope");
  ASSERT_EQ(exporter.Export(data.Sum()), ExportResult::kSuccess);

  ASSERT_TRUE(exporter.ForceFlush(std::chrono::microseconds{2 * 1000 * 1000}));
  ASSERT_FALSE(ReadFileContent(log_file).empty());

  // shutdown joins the background flush thread
  ASSERT_TRUE(exporter.Shutdown(std::chrono::microseconds{2 * 1000 * 1000}));
}

TEST(PrometheusFileExporterFactory, Create)
{
  const std::string directory = "prometheus_file_exporter_test_factory";
  const std::string log_file  = directory + "/factory.0.log";
  TestFileCleaner cleaner({log_file}, {directory});

  PrometheusFileExporterOptions options = MakeOptions(directory, "factory.%N.log");
  auto exporter = PrometheusFileExporterFactory::Create(options);
  ASSERT_TRUE(!!exporter);
  ASSERT_EQ(exporter->GetAggregationTemporality(metric_sdk::InstrumentType::kCounter),
            metric_sdk::AggregationTemporality::kCumulative);

  ScopedMetricData data("factory_scope");
  ASSERT_EQ(exporter->Export(data.Sum()), ExportResult::kSuccess);
  ASSERT_TRUE(exporter->Shutdown(std::chrono::microseconds{1000}));

  ASSERT_NE(ReadFileContent(log_file).find("factory_scope"), std::string::npos);
}
