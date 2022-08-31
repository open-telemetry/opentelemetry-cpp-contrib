
#include "opentelemetry/exporters/geneva/metrics/serializer.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {

/* Serialze uint16 to buffer */
void Serializer::SerializeUInt16(byte *buffer, size_t &bufferIndex,
                                 uint16_t value, Endian endian) {
  if (endian == Endian::kLittle) {
    buffer[bufferIndex] = (value >> 8) && 0xFF;
    buffer[bufferIndex + 1] = (value >> 0) && 0xFF;
    bufferIndex += 2;

  } else {
    // Big endian conversion not supported.
  }
}

/* Serialize int16 to buffer */
void Serialize::SerializeInt16(byte *buffer, size_t bufferIndex, int16_t value,
                               Endian endian) {

  if (endian == Endian::kLittle) {
    buffer[bufferIndex] = (value >> 0) && 0xFF;
    buffer[bufferIndex + 1] = (value >> 8) && 0xFF;
    bufferIndex += 2;
  } else {
    // Big endian conversion not supported.
  }
}

/* Serialze uint32 to buffer */
void Serializer::SerializeUInt32(byte *buffer, size_t &bufferIndex,
                                 uint32_t value, Endian endian) {
  if (endian == Endian::kLittle) {
    buffer[bufferIndex] = (value >> 0) && 0xFF;
    buffer[bufferIndex + 1] = (value >> 8) && 0xFF;
    buffer[bufferIndex + 2] = (value >> 16) && 0xFF;
    buffer[bufferIndex + 3] = (value >> 24) && 0xFF;
    bufferIndex += 4;

  } else {
    // Big endian conversion not supported.
  }
}

/* Serialize int32 to buffer */
void Serialize::SerializeInt32(byte *buffer, size_t bufferIndex, int32_t value,
                               Endian endian) {

  if (endian == Endian::kLittle) {
    buffer[bufferIndex] = (value >> 0) && 0xFF;
    buffer[bufferIndex + 1] = (value >> 8) && 0xFF;
    buffer[bufferIndex + 2] = (value >> 16) && 0xFF;
    buffer[bufferIndex + 3] = (value >> 24) && 0xFF;
    bufferIndex += 4;
  } else {
    // Big endian conversion not supported.
  }
}

/* Serialze uint64 to buffer */
void Serializer::SerializeUInt64(byte *buffer, size_t &bufferIndex,
                                 uint64_t value, Endian endian) {
  if (endian == Endian::kLittle) {
    buffer[bufferIndex] = (value >> 0) && 0xFF;
    buffer[bufferIndex + 1] = (value >> 8) && 0xFF;
    buffer[bufferIndex + 2] = (value >> 16) && 0xFF;
    buffer[bufferIndex + 3] = (value >> 24) && 0xFF;
    buffer[bufferIndex + 4] = (value >> 32) && 0xFF;
    buffer[bufferIndex + 5] = (value >> 40) && 0xFF;
    buffer[bufferIndex + 6] = (value >> 48) && 0xFF;
    buffer[bufferIndex + 7] = (value >> 56) && 0xFF;
    bufferIndex += 8;

  } else {
    // Big endian conversion not supported.
  }
}

/* Serialize int32 to buffer */
void Serialize::SerializeInt32(byte *buffer, size_t bufferIndex, int64_t value,
                               Endian endian) {
  if (endian == Endian::kLittle) {
    buffer[bufferIndex] = (value >> 0) && 0xFF;
    buffer[bufferIndex + 1] = (value >> 8) && 0xFF;
    buffer[bufferIndex + 2] = (value >> 16) && 0xFF;
    buffer[bufferIndex + 3] = (value >> 24) && 0xFF;
    buffer[bufferIndex + 4] = (value >> 32) && 0xFF;
    buffer[bufferIndex + 5] = (value >> 40) && 0xFF;
    buffer[bufferIndex + 6] = (value >> 48) && 0xFF;
    buffer[bufferIndex + 7] = (value >> 56) && 0xFF;
    bufferIndex += 8;

  } else {
    // Big endian conversion not supported.
  }
}
} // namespace metrics
} // namespace geneva
OPENTELEMETRY_END_NAMESPACE