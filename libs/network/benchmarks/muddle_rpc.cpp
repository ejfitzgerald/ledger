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
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/service/protocol.hpp"
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
using fetch::muddle::rpc::Server;
using fetch::muddle::rpc::Client;
using fetch::network::NetworkManager;
using fetch::network::Uri;

constexpr uint16_t SERVICE  = 1;
constexpr uint16_t CHANNEL  = 2;
constexpr uint64_t PROTOCOL = 3;

class DummyProtocol : public fetch::service::Protocol
{
public:

  enum
  {
    CALL = 4
  };

  DummyProtocol()
  {
    Expose(CALL, this, &DummyProtocol::Handler);
  }

private:

  bool Handler()
  {
    return true;
  }
};

void MuddleRpc(benchmark::State &state)
{
  NetworkManager nm{"bench", 2};
  nm.Start();

  auto id1 = std::make_shared<ECDSASigner>();
  auto id2 = std::make_shared<ECDSASigner>();

  NetworkId const network_id{"PERF"};

  Muddle muddle1{network_id, id1, nm};
  Muddle muddle2{network_id, id2, nm};

  Server rpc_server{muddle1.AsEndpoint(), SERVICE, CHANNEL};
  Client rpc_client{"client", muddle2.AsEndpoint(), SERVICE, CHANNEL};

  // add the protocol to the server
  DummyProtocol server_proto{};
  rpc_server.Add(PROTOCOL, &server_proto);

  muddle1.Start({8000});
  muddle2.Start({8010}, {Uri{"tcp://127.0.0.1:8000"}});

  // wait for the connection to be established
  WaitFor([&]() {
    return (muddle1.AsEndpoint().GetDirectlyConnectedPeers().size() == 1) &&
           (muddle2.AsEndpoint().GetDirectlyConnectedPeers().size() == 1);
  });

  auto const server_address = id1->identity().identifier();

  for (auto _ : state)
  {
    auto prom = rpc_client.CallSpecificAddress(server_address, PROTOCOL, DummyProtocol::CALL);
    prom->Wait();
  }

  muddle2.Stop();
  muddle1.Stop();
  nm.Stop();
}

} // namespace

BENCHMARK(MuddleRpc);
