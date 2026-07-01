// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/metrics/async_instruments.h"
class TestLib
{
public:
  static void create_process_cpu_time_observable_counter();
  static void create_process_cpu_utilization_observable_gauge();
  static void create_process_memory_usage_observable_gauge();
  static void create_process_memory_virtual_observable_gauge();
  static void create_process_disk_io_observable_gauge();
  static void create_process_network_io_observable_gauge();
  static void create_process_threads_observable_gauge();
  static void create_process_open_files_observable_gauge();
  static void create_process_context_switches_observable_gauge();
private:
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> cpu_time_obserable_counter_;
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> cpu_utilization_obserable_gauge_;
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> memory_usage_obserable_gauge_;
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> memory_virtual_obserable_gauge_;
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> disk_io_obserable_gauge_;
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> network_io_obserable_gauge_;
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> threads_obserable_gauge_;
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> open_files_obserable_gauge_;
  static opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument> context_switches_obserable_gauge_;
};
