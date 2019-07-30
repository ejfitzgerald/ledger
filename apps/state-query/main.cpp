//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "state_query.hpp"

#include "core/logging.hpp"
#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "network/management/network_manager.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/network_id.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/uri.hpp"
#include "storage/resource_mapper.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using fetch::byte_array::ByteArray;
using fetch::crypto::ECDSASigner;
using fetch::network::NetworkManager;
using fetch::network::Uri;
using fetch::muddle::Muddle;
using fetch::muddle::NetworkId;
using fetch::muddle::MuddleEndpoint;
using fetch::muddle::rpc::Client;
using fetch::storage::ResourceAddress;

using Clock = std::chrono::high_resolution_clock;

static constexpr char const *LOGGING_NAME = "state-query";
static NetworkId const NETWORK{"ISRD"};

template <typename F>
void WaitFor(F &&event)
{
  using namespace std::chrono_literals;
  using std::this_thread::sleep_for;

  for (;;)
  {
    // exit the wait loop
    if (event())
    {
      break;
    }

    sleep_for(10ms);
  }
}

void run(StateQuery &query)
{
  std::size_t const ITERATIONS = 000;

  FETCH_LOG_INFO(LOGGING_NAME, "Generating addresses...");

  std::vector<ResourceAddress> addresses;
  addresses.reserve(ITERATIONS);
  for (std::size_t i = 0; i < ITERATIONS; ++i)
  {
    addresses.emplace_back(ResourceAddress{"foo.bar." + std::to_string(i)});
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Generating addresses...complete");

  // random value
  ByteArray value;
  value.Resize(256);

  auto const start = Clock::now();

  for (std::size_t i = 0; i < ITERATIONS; ++i)
  {
    query.Set(addresses[i], value);
  }

  for (std::size_t i = 0; i < ITERATIONS; ++i)
  {
    query.Get(addresses[i]);
  }

  auto const delta = Clock::now() - start;

  double const total_time =
                   std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
  double const request_time = (total_time * 1e6) / static_cast<double>(ITERATIONS);

  std::cout << "  Total Time: " << total_time << " s" << std::endl;
  std::cout << "Request Time: " << request_time << " us" << std::endl;
}

int main()
{
  NetworkManager nm{"main", 1};
  nm.Start();

  auto certificate = std::make_shared<ECDSASigner>();

  Muddle muddle{NETWORK, certificate, nm};
  muddle.Start({}, {Uri{"tcp://127.0.0.1:8011"}});

  auto &endpoint = muddle.AsEndpoint();

  // wait until we are connected
  WaitFor([&endpoint]() { return !endpoint.GetDirectlyConnectedPeers().empty(); });

  try
  {
    StateQuery query{endpoint};

    // run the handler
    run(query);
  }
  catch (std::exception const &ex)
  {
    std::cerr << "Error: " << ex.what() << std::endl;
  }

  muddle.Stop();
  nm.Stop();

  return EXIT_SUCCESS;
}