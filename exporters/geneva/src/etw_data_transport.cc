// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/exporters/geneva/metrics/etw_data_transport.h"
#include "opentelemetry/exporters/geneva/metrics/macros.h"
#include "opentelemetry/exporters/geneva/metrics/uuid.h"

#include "opentelemetry/exporters/geneva/metrics/macros.h"

ETWDataTransport::ETWDataTransport(const std::string &etw_provider) {
  UUID guid =
      (etw_provider.rfind("{", 0) == 0)
          ? UUID(etw_provider.c_str())             /// It's a ProviderGUID
          : GetProviderGuid(etw_provider.c_str()); // It's a ProviderName

  auto provider_guid = guid.to_GUID();
  auto status = ::EventRegister(&provider_guid, NULL, NULL, &provider_handle_);
  if (status != ERROR_SUCCESS) {
    LOG_ERROR("ETWDataTransport:: Failed to initialize the ETW provider.  "
              "Metrics will not be published");
    provider_handle_ = INVALID_HANDLE;
  }
}

bool ETWDataTransport::Connect() {
  // connection is already established in constructor. Check if it is still
  // valid.
  if (provider_handle_ == INVALID_HANDLE) {
    LOG_ERROR("ETWDataTransport:: Failed to initialize the ETW provider.  "
              "Metrics will not be published");
    return true;
  }
  return true;
}

bool ETWDataTransport::Send(MetricsEventType event_type, const char *data,
                            uint16_t length) {
  if (provider_handle_ == INVALID_HANDLE) {
    LOG_ERROR("ETWDataTransport:: ETW Provider Handle is not valid. Metrics is "
              "dropped");
    return false;
  }
  const unsigned int descriptorSize = 1;
  EVENT_DATA_DESCRIPTOR dataDescriptor[descriptorSize];
  ::ZeroMemory(&dataDescriptor, sizeof(dataDescriptor));
  ::EventDataDescCreate(&dataDescriptor[0], data, length);

  EVENT_DESCRIPTOR evtDescriptor;
  ::ZeroMemory(&evtDescriptor, sizeof(EVENT_DESCRIPTOR));
  evtDescriptor.Version = 1;
  evtDescriptor.Version = 0;
  evtDescriptor.Id = static_cast<unsigned short>(eventType);
  auto result = ::EventWrite(provider_handle_, &evtDescriptor, descriptorSize,
                             dataDescriptor);
  if (result != ERROR_SUCCESS) {
    LOG_ERROR("ETWDataTransport:: Failed to publish metric to ETW. Error: %d",
              result);
    return false;
  }
  return true;
}

bool ETWDataTransport::Disconnect() {
  // provider is deregistered in destructor.
  return true;
}

ETWDataTransport::~ETWDataTransport() {
  if (provider_handle_ != INVALID_HANDLE) {
    ::EventUnregister(provider_handle_);
  }
}