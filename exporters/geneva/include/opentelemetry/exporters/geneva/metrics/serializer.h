// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

# include "opentelemetry/sdk/metrics/metric_exporter.h"
# include "opentelemetry/exporters/geneva/metrics/exporter_options.h"
# include "opentelemetry/exporters/ge/common/socket_tools.h"
# include "opentelemetry/common/spin_lock_mutex.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{
    typedef uint8_t byte;

enum class Endian {
    kLittle = 0,
    kBig = 1,
 };


    class BinarySerializer
    {
        public:
        static void SerializeUInt16(byte* buffer, size_t bufferIndex,  uint16_t value, Endian endian = Endian::kLittle);
        static void SerializeInt16(byte* buffer, size_t bufferIndex, int16_t value, Endian endian = Endian::kLittle);
        static void SerializeUInt32(byte* buffer, size_t bufferIndex, uint32_t value, Endian endian = Endian::kLittle);
        static void SerializeUInt64(byte* buffer, size_t bufferIndex, uint64_t value, Endian endian = Endian::kLittle);
    };
}
}
}
OPENTELEMETRY_END_NAMESPACE