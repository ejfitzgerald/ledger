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

#include "core/byte_array/const_byte_array.hpp"
#include "beacon/beacon_service.hpp"
#include "ledger/chain/main_chain.hpp"

#include <iostream>

using fetch::byte_array::ConstByteArray;
using fetch::ledger::MainChain;

int main()
{
  /// Arg!
  fetch::crypto::mcl::details::MCLInitialiser();

  // Mainnet specific
  auto const genesis_hash =
      ConstByteArray{"64e8f7b5d1505f02293b7d5871e6deffe5971596caf6f17fe626c8899c780810"}.FromHex();
  auto const genesis_merkle_root =
      ConstByteArray{"e7f41d14ac9657a34b0aeb09aa635291dce52e8f067caa057e8dd4e620d0c9f4"}.FromHex();
  auto const target_hash =
      ConstByteArray{"248b38fd6a97e13345a15ba40737d2d2ad64d99f943cc7b981718c11c2b5e096"}.FromHex();

  // ensure the chain constants are golden
  fetch::chain::SetGenesisDigest(genesis_hash);
  fetch::chain::SetGenesisMerkleRoot(genesis_merkle_root);

  // load the database in the same way as the standard ledger
  MainChain::Config cfg{};
  cfg.fast_load = true; // unsafe!
  MainChain chain{MainChain::Mode::LOAD_PERSISTENT_DB, cfg};

  auto block = chain.GetBlock(target_hash);
  if (!block) {
    std::cerr << "Unable to lookup target block" << std::endl;
  }

  // reset the beacon service back down to the
  fetch::beacon::BeaconService::DebugState(); // print the state before
  fetch::beacon::BeaconService::ResetStateToBlock(block->block_entropy);
  fetch::beacon::BeaconService::DebugState(); // print the state after

  return 0;
}