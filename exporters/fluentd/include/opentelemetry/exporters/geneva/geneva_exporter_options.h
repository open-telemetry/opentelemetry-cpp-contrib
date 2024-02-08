// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <opentelemetry/exporters/fluentd/common/fluentd_common.h>
#include<string>

OPENTELEMETRY_BEGIN_NAMESPACE

namespace exporters {

namespace geneva {

constexpr const char *kUnixDomainScheme = "unix://";
constexpr const size_t kUnixDomainSchemeLength = 7; // length of "unix://"
struct GenevaExporterOptions {

    /** socker path for unix domain socket. Should start with unix://
    *  Example unix:///tmp/.socket_geneva_exporter
    */
    std::string socket_endpoint ;

    /* number of retries before failing */

    size_t retry_count = 2;

    /**
    * The maximum buffer/queue size. After the size is reached, spans/logs are
    * dropped.
    */
    size_t max_queue_size = 2048; // max buffer size dropping logs/spans

    /* The time interval between two consecutive exports. */
    std::chrono::milliseconds schedule_delay_millis = std::chrono::milliseconds(5000);

    /**
    * The maximum batch size of every export. It must be smaller or
    * equal to max_queue_size.
    */
    size_t max_export_batch_size = 512;
};

}
}
OPENTELEMETRY_END_NAMESPACE

