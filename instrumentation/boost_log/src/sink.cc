/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <opentelemetry/instrumentation/boost_log/sink.h>

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/trace/semantic_conventions.h>

#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/type_dispatch/date_time_types.hpp>

namespace opentelemetry
{
namespace instrumentation
{
namespace boost_log
{

static bool ToTimestampDefault(const boost::log::record_view &record,
                               std::chrono::system_clock::time_point &value) noexcept
{
  using namespace std::chrono;
  static constexpr boost::posix_time::ptime kEpochTime(boost::gregorian::date(1970, 1, 1));
  static constexpr boost::posix_time::ptime kInvalid{};

  auto timestamp =
      boost::log::extract_or_default<boost::posix_time::ptime>(record["TimeStamp"], kInvalid);
  value = system_clock::time_point(nanoseconds((timestamp - kEpochTime).total_nanoseconds()));
  return timestamp != kInvalid;
}

static bool ToThreadIdDefault(const boost::log::record_view &record, std::string &value) noexcept
{
  static constexpr boost::log::aux::thread::id kInvalid{};

  auto thread_id =
      boost::log::extract_or_default<boost::log::aux::thread::id>(record["ThreadID"], kInvalid);

  std::stringstream ss;
  ss << thread_id;
  value = ss.str();
  return thread_id != kInvalid;
}

static bool ToFileNameDefault(const boost::log::record_view &record, std::string &value) noexcept
{
  value = boost::log::extract_or_default<std::string>(record["FileName"], std::string{});
  return !value.empty();
}

static bool ToFuncNameDefault(const boost::log::record_view &record, std::string &value) noexcept
{
  value = boost::log::extract_or_default<std::string>(record["FunctionName"], std::string{});
  return !value.empty();
}

static bool ToLineNumberDefault(const boost::log::record_view &record, int &value) noexcept
{
  static constexpr int kInvalid = -1;
  value = boost::log::extract_or_default<int>(record["LineNumber"], kInvalid);
  return value != kInvalid;
}

OpenTelemetrySinkBackend::OpenTelemetrySinkBackend(const ValueMappers &mappers) noexcept
{
  mappers_.ToSeverity =
      mappers.ToSeverity ? mappers.ToSeverity : [](const boost::log::record_view &record) {
        using boost::log::trivial::severity_level;
        return levelToSeverity(boost::log::extract_or_default<severity_level>(
            record["Severity"], static_cast<severity_level>(-1)));
      };
  mappers_.ToTimeStamp = mappers.ToTimeStamp ? mappers.ToTimeStamp : ToTimestampDefault;
  mappers_.ToThreadId  = mappers.ToThreadId ? mappers.ToThreadId : ToThreadIdDefault;
  mappers_.ToCodeFile  = mappers.ToCodeFile ? mappers.ToCodeFile : ToFileNameDefault;
  mappers_.ToCodeFunc  = mappers.ToCodeFunc ? mappers.ToCodeFunc : ToFuncNameDefault;
  mappers_.ToCodeLine  = mappers.ToCodeLine ? mappers.ToCodeLine : ToLineNumberDefault;

  using namespace opentelemetry::trace::SemanticConventions;
  using opentelemetry::logs::LogRecord;
  using timestamp_t = std::chrono::system_clock::time_point;

  set_timestamp_if_valid_ = {[](LogRecord *, const timestamp_t &) {},
                             [](LogRecord *log_record, const timestamp_t &timestamp) {
                               log_record->SetTimestamp(timestamp);
                             }};

  set_thread_id_if_valid_ = {[](LogRecord *, const std::string &) {},
                             [](LogRecord *log_record, const std::string &thread_id) {
                               log_record->SetAttribute(kThreadId, thread_id);
                             }};

  set_file_path_if_valid_ = {[](LogRecord *, const std::string &) {},
                             [](LogRecord *log_record, const std::string &file_name) {
                               log_record->SetAttribute(kCodeFilepath, file_name);
                             }};

  set_func_name_if_valid_ = {[](LogRecord *, const std::string &) {},
                             [](LogRecord *log_record, const std::string &func_name) {
                               log_record->SetAttribute(kCodeFunction, func_name);
                             }};

  set_code_line_if_valid_ = {[](LogRecord *, int) {},
                             [](LogRecord *log_record, int code_line) {
                               log_record->SetAttribute(kCodeLineno, code_line);
                             }};
}

void OpenTelemetrySinkBackend::consume(const boost::log::record_view &record)
{
  static constexpr auto kLoggerName  = "Boost logger";
  static constexpr auto kLibraryName = "Boost.Log";

  auto provider   = opentelemetry::logs::Provider::GetLoggerProvider();
  auto logger     = provider->GetLogger(kLoggerName, kLibraryName, libraryVersion());
  auto log_record = logger->CreateLogRecord();

  if (log_record)
  {
    log_record->SetBody(
        boost::log::extract_or_default<std::string>(record["Message"], std::string{}));
    log_record->SetSeverity(mappers_.ToSeverity(record));

    std::chrono::system_clock::time_point timestamp;
    set_timestamp_if_valid_[mappers_.ToTimeStamp(record, timestamp)](log_record.get(), timestamp);

    std::string thread_id;
    set_thread_id_if_valid_[mappers_.ToThreadId(record, thread_id)](log_record.get(), thread_id);

    std::string file_name;
    set_file_path_if_valid_[mappers_.ToCodeFile(record, file_name)](log_record.get(), file_name);

    std::string func_name;
    set_func_name_if_valid_[mappers_.ToCodeFunc(record, func_name)](log_record.get(), func_name);

    int code_line;
    set_code_line_if_valid_[mappers_.ToCodeLine(record, code_line)](log_record.get(), code_line);

    logger->EmitLogRecord(std::move(log_record));
  }
}

}  // namespace boost_log
}  // namespace instrumentation
}  // namespace opentelemetry
