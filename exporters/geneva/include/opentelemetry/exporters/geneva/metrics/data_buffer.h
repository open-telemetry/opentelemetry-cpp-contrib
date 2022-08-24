// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/version.h"
#include<vector>
#include<string>
#include <stdint.h>

#pragma once

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter
{
namespace geneva
{
namespace metrics
{

constexpr size_t kInitialBufferSize = 265;
using ByteVector = std::vector<unsigned char>;
class DataBuffer
{
    public:
        DataBuffer();
        void SerializeEncodedString(const std::string&);
                
        template <class T>
        void SerializeInt(T value) {
            ResizeBufferIfNeeded(sizeof(T));
            *(reinterpret_cast<T*>(buffer_->data() + current_buffer_index_)) = value;
            current_buffer_index_+= sizeof(T);          
        }

        uint64_t GetSize() const {
            return current_buffer_index_;
        }

        void* GetData() const {
            return buffer_->data();
        }
    private:
        ByteVector* buffer_;
        uint64_t current_buffer_index_;
        void ResizeBufferIfNeeded( uint64_t additional_capacity);
};

}
}
}
OPENTELEMETRY_END_NAMESPACE