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

#include "wait_for.hpp"

#include "core/byte_array/byte_array.hpp"
#include "crypto/ecdsa.hpp"
#include "network/management/network_manager.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/network_id.hpp"
#include "network/uri.hpp"

#include "benchmark/benchmark.h"

#include <memory>

namespace {

using namespace std::chrono_literals;

using std::this_thread::sleep_for;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;
using fetch::crypto::ECDSASigner;
using fetch::muddle::Muddle;
using fetch::muddle::MuddleEndpoint;
using fetch::muddle::NetworkId;
using fetch::network::NetworkManager;
using fetch::network::Uri;

constexpr uint16_t SERVICE = 1;
constexpr uint16_t CHANNEL = 2;


void MuddleExchange(benchmark::State &state)
{
  NetworkManager nm{"bench", 2};
  nm.Start();

  auto id1 = std::make_shared<ECDSASigner>();
  auto id2 = std::make_shared<ECDSASigner>();

  NetworkId const network_id{"PERF"};

  Muddle muddle1{network_id, id1, nm};
  Muddle muddle2{network_id, id2, nm};

  auto sub = muddle1.AsEndpoint().Subscribe(SERVICE, CHANNEL);
  sub->SetMessageHandler([&muddle1](ConstByteArray const &from, uint16_t service, uint16_t channel,
                                    uint16_t counter, ConstByteArray const &payload,
                                    ConstByteArray const & /*transmitter*/) {
    // simple echo handler
    muddle1.AsEndpoint().Send(from, service, channel, counter, payload);
  });

  muddle1.Start({8000});
  muddle2.Start({8010}, {Uri{"tcp://127.0.0.1:8000"}});

  // wait for the connection to be established
  WaitFor([&]() {
    return (muddle1.AsEndpoint().GetDirectlyConnectedPeers().size() == 1) &&
           (muddle2.AsEndpoint().GetDirectlyConnectedPeers().size() == 1);
  });

  ByteArray payload;
  payload.Resize(28);

  for (auto _ : state)
  {
    auto promise = muddle2.AsEndpoint().Exchange(id1->identity().identifier(), SERVICE, CHANNEL, payload);
    promise.Wait();
  }

  muddle2.Stop();
  muddle1.Stop();
  nm.Stop();
}

} // namespace

BENCHMARK(MuddleExchange);
