// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

class TestLib
{
public:
  static void create_process_cpu_time_observable_counter();
  static void create_process_cpu_utilization_observable_gauge();
  static void create_process_memory_usage_observable_gauge();
  static void create_process_memory_virtual_observable_gauge();
  static void create_process_disk_io_observable_counter();
  static void create_process_network_io_observable_counter();
  static void create_process_threads_observable_counter();
  static void create_process_open_files_observable_gauge();
  static void create_process_context_switches_observable_counter();
};
