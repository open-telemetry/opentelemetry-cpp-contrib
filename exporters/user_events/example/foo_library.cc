// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_LOGS_PREVIEW

#  include "opentelemetry/logs/event_id.h"
#  include "opentelemetry/logs/provider.h"
#  include "opentelemetry/sdk/version/version.h"

namespace logs  = opentelemetry::logs;
namespace nostd = opentelemetry::nostd;

const logs::EventId fruit_sell_event_id{0x1, "FruitCompany.SalesDepartment.SellFruit"};
const logs::EventId fruit_not_found_event_id{0x2, "FruitCompany.SalesDepartment.FruitNotFound"};

std::map<std::string, double> fruit_prices = {{"apple", 1.1}, {"orange", 2.2}, {"banana", 3.3}};

namespace
{
nostd::shared_ptr<logs::Logger> get_logger()
{
  auto provider = logs::Provider::GetLoggerProvider();
  return provider->GetLogger("foo_library_logger", "foo_library");
}
}  // namespace

void sell_fruit(std::string_view fruit)
{
  auto logger = get_logger();

  if (fruit_prices.find(std::string{fruit}) == fruit_prices.end())
  {
    logger->Error(
      fruit_not_found_event_id,
      "Fruit {name} not found",
      opentelemetry::common::MakeAttributes({{"name", fruit.data()}}));

    return;
  }

  logger->Trace(
      fruit_sell_event_id,
      "Selling fruit {name} with {price}",
      opentelemetry::common::MakeAttributes({{"name", fruit.data()}, {"price", fruit_prices[fruit.data()]}}));
}

#endif
