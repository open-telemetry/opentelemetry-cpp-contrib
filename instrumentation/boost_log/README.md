# Boost log OpenTelemetry sink backend

## Features

- Supports Boost.Log sink backend mechanism and allows for some customizations
- Supports OpenTelemetry SDK without any changes

## Requirements

- Current release tested only with Ubuntu 20.04.6 LTS
- OpenTelemetry >= v1.12.0 
- Boost.Log >= v1.73.0

### Usage

Please see below for [manual build](#build). Otherwise please use one of the [released versions](https://github.com/open-telemetry/opentelemetry-cpp-contrib/releases).

### Configuration

The OpenTelemetry sink can be configured in much the same way as any other Boost log sink backend.

The simplest scenario is by creating a logger with the `OpenTelemetrySinkBackend`:

```cpp
#include <opentelemetry/instrumentation/boost_log/sink.h>

using opentelemetry::instrumentation::boost_log::OpenTelemetrySinkBackend;

auto backend = boost::make_shared<OpenTelemetrySinkBackend>();
auto sink    = boost::make_shared<boost::log::sinks::synchronous_sink<OpenTelemetrySinkBackend>>(backend);
boost::log::core::get()->add_sink(sink);
```

This will create a backend with the following assumptions about the attributes it collects, more precisely:

| Keyword      | Type                                |
|--------------|-------------------------------------|
| Severity     | boost::log::trivial::severity_level |
| TimeStamp    | boost::posix_time::ptime            |
| ThreadID     | boost::log::aux::thread::id         |
| FileName     | std::string                         |
| FunctionName | std::string                         |
| LineNumber   | int                                 |

If, however, one or more of these attributes have a different name or type, it is possible to communicate this to the backend via the `ValueMappers` struct. It contains a function for each attribute described above, with their signatures as follows:

```cpp
opentelemetry::logs::Severity ToSeverity(const boost::log::record_view &);
bool ToTimestamp(const boost::log::record_view &record, std::chrono::system_clock::time_point &value);
bool ToThreadId(const boost::log::record_view &record, std::string &value);
bool ToFileName(const boost::log::record_view &record, std::string &value);
bool ToFuncName(const boost::log::record_view &record, std::string &value);
bool ToLineNumber(const boost::log::record_view &record, int &value);
```

With the exception of `ToSeverity`, which is always required to return some OpenTelemetry severity type, the requirements for the other methods are more relaxed, essentially making them optional. They take an additional *out* parameter **value** and return a boolean to indicate success or failure.

As an example of usage, below is one way to create the OTel sink backend with a custom `ToSeverity` method that maps to an enum class `CustomSeverity` and reads the value from the attribute *"LogLevel"* instead of the default *"Severity"*:

```cpp
enum class CustomSeverity
{
  kRed, kOrange, kYellow, kGreen, kBlue, kIndigo, kViolet
};

opentelemetry::instrumentation::boost_log::ValueMappers mappers;
mappers.ToSeverity = [](const boost::log::record_view &record) {
  if (const auto &result = boost::log::extract<CustomSeverity>(record["Severity"]))
  {
    switch (result.get())
    {
      case CustomSeverity::kRed:
        return opentelemetry::logs::Severity::kFatal;
      case CustomSeverity::kOrange:
        return opentelemetry::logs::Severity::kError;
      case CustomSeverity::kYellow:
        return opentelemetry::logs::Severity::kWarn;
      case CustomSeverity::kGreen:
        return opentelemetry::logs::Severity::kInfo;
      case CustomSeverity::kBlue:
        return opentelemetry::logs::Severity::kDebug;
      case CustomSeverity::kIndigo:
        return opentelemetry::logs::Severity::kTrace;
      case CustomSeverity::kViolet:
        [[fallthrough]];
      default: 
        return opentelemetry::logs::Severity::kInvalid;
    }
  }

  return opentelemetry::logs::Severity::kInvalid;
};

auto backend = boost::make_shared<OpenTelemetrySinkBackend>(mappers);
auto sink    = boost::make_shared<boost::log::sinks::synchronous_sink<OpenTelemetrySinkBackend>>(backend);
boost::log::core::get()->add_sink(sink);
```

Another, although somewhat impractical, example of setting a custom timestamp:

```cpp
mappers.ToTimeStamp = [](const boost::log::record_view &,
                         std::chrono::system_clock::time_point &value) {
  value = std::chrono::system_clock::now() - std::chrono::milliseconds(42);
  return true;
};
```

For more details, refer to the [examples](#examples) section.

## Development

### Requirements

- C++14
- CMake 3.x
- [OpenTelemetry-cpp](https://github.com/open-telemetry/opentelemetry-cpp)
- [Boost.Log](https://github.com/boostorg/log)
- vcpkg **_(optional)_**

### Build
As a preparation step, both dependencies need to be built and available in the development environment. This can be a manual build, by following the instructions for the corresponding package, or one could opt to use a package management system such as _vcpkg_ or _conan_.

Assuming the packages are available on the system, configure CMake as usual:

```bash
mkdir build
cd build
cmake [path/to/opentelemetry-cpp-contrib]/instrumentation/boost_log -DBUILD_SHARED_LIBS=ON
make
```

Optionally, if the packages were provided via vcpkg, pass in to the _cmake_ command above the flag `-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` where VCPKG_ROOT is where vcpkg was installed. 

Now, simply link the target source (i.e., main.cc in the example below) against _boost_log_ as well as _opentelemetry_boost_log_sink_:

```bash
g++ main.cc -lboost_log -lopentelemetry_boost_log_sink -o main
```

### Installation ###

When configuring the build, if the flag `-DOPENTELEMETRY_INSTALL=ON` is passed, CMake will ensure to set up the install scripts. Once the build succeeds, running `make install` will make the sink header(s) and library available under the usr include and lib directories, respectively.

### Testing

A small suite of unit tests is available under the `test` directory. It can be enabled by passing `-DBUILD_TESTING=ON` when configuring CMake, which will generate an executable called **sink_test**.

### Examples

An example executable is available to test the functionality of the sink. For ease of setup, it uses the _OStreamLogRecordExporter_ to display the contents of the intercepted Boost log message as an OTel _LogRecord_. It can be generated by adding `-DWITH_EXAMPLES=ON` when configuring CMake, which will ultimately produce the **otel_sink_example** executable.

Running  `./otel_sink_example` would produce an output similar to following excerpts.

```
{
  timestamp          : 1704766376214174000
  observed_timestamp : 1704766376214298462
  severity_num       : 0
  severity_text      : INVALID
  body               : Test simplest message
  resource           :
    service.name: unknown_service
    telemetry.sdk.version: 1.12.0
    telemetry.sdk.name: opentelemetry
    telemetry.sdk.language: cpp
  attributes         :
    thread.id: 0x00007f5d78f932c0
  event_id           : 0
  event_name         :
  trace_id           : 3b74715eaab1e0026feff669ee7f27a1
  span_id            : a51c93eef9e7af66
  trace_flags        : 01
  scope              :
    name             : Boost.Log
    version          : 1.83.0
    schema_url       :
    attributes       :
}
```
The above is an example of calling BOOST_LOG with a simple logger that is not severity-aware and, as such, the severity is invalid. 

```
{
  timestamp          : 1704766376214570000
  observed_timestamp : 1704766376214576298
  severity_num       : 9
  severity_text      : INFO
  body               : Test message with severity
  resource           :
    service.name: unknown_service
    telemetry.sdk.version: 1.12.0
    telemetry.sdk.name: opentelemetry
    telemetry.sdk.language: cpp
  attributes         :
    thread.id: 0x00007f5d78f932c0
  event_id           : 0
  event_name         :
  trace_id           : 3b74715eaab1e0026feff669ee7f27a1
  span_id            : a51c93eef9e7af66
  trace_flags        : 01
  scope              :
    name             : Boost.Log
    version          : 1.83.0
    schema_url       :
    attributes       :
}
```
This example is the result of calling BOOST_LOG_SEV with a severity logger and  trivial severity level.  

```
{
  timestamp          : 1704766376214627000
  observed_timestamp : 1704766376214643807
  severity_num       : 5
  severity_text      : DEBUG
  body               : Test message with source location
  resource           :
    service.name: unknown_service
    telemetry.sdk.version: 1.12.0
    telemetry.sdk.name: opentelemetry
    telemetry.sdk.language: cpp
  attributes         :
    code.lineno: 90
    code.function: main
    code.filepath: /otel-contrib/instrumentation/boost_log/example/main.cc
    thread.id: 0x00007f5d78f932c0
  event_id           : 0
  event_name         :
  trace_id           : 3b74715eaab1e0026feff669ee7f27a1
  span_id            : a51c93eef9e7af66
  trace_flags        : 01
  scope              :
    name             : Boost.Log
    version          : 1.83.0
    schema_url       :
    attributes       :
}
```

The above calls the same macro as in the previous example, with the values added for the source location (line / function / file).

The default behavior to log the source location is by adding attributes values to the BOOST_LOG* macro with the following keywords:

```cpp
BOOST_LOG_SEV(logger, boost::log::trivial::debug)
    << boost::log::add_value("FileName", __FILE__)
    << boost::log::add_value("FunctionName", __FUNCTION__)
    << boost::log::add_value("LineNumber", __LINE__) << "Test message with source location";
```

If logging with a different keyword or type, a custom mapping method has to be provided when instantiating the backend sink, as shown in [configuration](#configuration) section.

For all the examples, the common attributes were enabled with `boost::log::add_common_attributes();`. Otherwise, the timestamp would not be populated and thread ID would not be included at all in the exported log record.

This example will also output a span with the same ID as the log record, to showcase how the two signals can be correlated via trace and/or span ID.
