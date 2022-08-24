// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/geneva/metrics/data_buffer.h"
#include<cstring>

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{
    DataBuffer::DataBuffer()
    {
        buffer_ = new std::vector<unsigned char>(kInitialBufferSize);
        current_buffer_index_ = 0;
    }

    void DataBuffer::SerializeEncodedString(const std::string& str)
    {
        auto size = str.size();
        ResizeBufferIfNeeded(sizeof(uint16_t) + (size == 0 ? 1 : size)); // The length + the string or at least 1 for a null placeholder
        SerializeInt<uint16_t>((static_cast<uint16_t>(size)));

        if (size > 0 ){
            memcpy(buffer_->data() + current_buffer_index_, str.c_str(), size);
        }
    }
    
    void DataBuffer::ResizeBufferIfNeeded( uint64_t additional_capacity)
    {
        auto target_capacity = additional_capacity + current_buffer_index_;
        if (buffer_->capacity() < target_capacity)
        {
            buffer_->resize(target_capacity + kInitialBufferSize);
        }
    }
}
}
}
OPENTELEMETRY_END_NAMESPACE