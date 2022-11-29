// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/geneva/metrics/etw_data_transport.h"
#include "opentelemetry/exporters/geneva/metrics/macros.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace exporter {
namespace geneva {
namespace metrics {
#define GUID_FORMAT                                                            \
  "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX"
#define GUID_ARG(guid)                                                         \
  (guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], (guid).Data4[1],  \
      (guid).Data4[2], (guid).Data4[3], (guid).Data4[4], (guid).Data4[5],      \
      (guid).Data4[6], (guid).Data4[7]

ETWDataTransport::ETWDataTransport(const size_t offset_to_skip)
    : offset_to_skip_{offset_to_skip} {
  auto status =
      ::EventRegister(&kMDMProviderGUID, NULL, NULL, &provider_handle_);
  if (status != ERROR_SUCCESS) {
    LOG_ERROR("ETWDataTransport:: Failed to initialize the ETW provider.  "
              "Metrics will not be published, Provider ID: {" GUID_FORMAT "}",
              GUID_ARG(kMDMProviderGUID));
    provider_handle_ = INVALID_HANDLE;
  }
}

bool ETWDataTransport::Connect() noexcept {
  // connection is already established in constructor. Check if it is still
  // valid.
  if (provider_handle_ == INVALID_HANDLE) {
    LOG_ERROR("ETWDataTransport:: Failed to initialize the ETW provider.  "
              "Metrics will not be published");
    return false;
  }
  return true;
}

bool ETWDataTransport::Send(MetricsEventType event_type, const char *data,
                            uint16_t length) noexcept {
  if (provider_handle_ == INVALID_HANDLE) {
    LOG_ERROR("ETWDataTransport:: ETW Provider Handle is not valid. Metrics is "
              "dropped");
    return false;
  }
  const unsigned int descriptorSize = 1;
  EVENT_DATA_DESCRIPTOR dataDescriptor[descriptorSize];
  ::ZeroMemory(&dataDescriptor, sizeof(dataDescriptor));
  // skip the event_id and the payload length (as expected by the ETW listener)
  ::EventDataDescCreate(&dataDescriptor[0], data + offset_to_skip_,
                        length - offset_to_skip_);

  EVENT_DESCRIPTOR evtDescriptor;
  ::ZeroMemory(&evtDescriptor, sizeof(EVENT_DESCRIPTOR));
  evtDescriptor.Version = 1;
  evtDescriptor.Version = 0;
  evtDescriptor.Id = static_cast<unsigned short>(event_type);
  auto result = ::EventWrite(provider_handle_, &evtDescriptor, descriptorSize,
                             dataDescriptor);
  if (result != ERROR_SUCCESS) {
    LOG_ERROR("ETWDataTransport:: Failed to publish metric to ETW. Error: %d",
              result);
    return false;
  }
  return true;
}

bool ETWDataTransport::Disconnect() noexcept {
  // provider is deregistered in destructor.
  return true;
}

ETWDataTransport::~ETWDataTransport() {
  if (provider_handle_ != INVALID_HANDLE) {
    ::EventUnregister(provider_handle_);
    provider_handle_ = INVALID_HANDLE;
  }
}

} // namespace metrics
} // namespace geneva
} // namespace exporter
OPENTELEMETRY_END_NAMESPACE