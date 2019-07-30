#pragma once
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

#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "network/muddle/rpc/client.hpp"

namespace fetch {
namespace muddle {

class MuddleEndpoint;

} // namespace muddle
} // namespace fetch

class StateQuery : public fetch::ledger::StorageInterface
{
public:
  using MuddleEndpoint = fetch::muddle::MuddleEndpoint;

  // Construction / Destruction
  explicit StateQuery(MuddleEndpoint &endpoint);
  StateQuery(StateQuery const &) = delete;
  StateQuery(StateQuery &&) = delete;
  ~StateQuery() override = default;

  /// @name State Interface
  /// @{
  Document Get(ResourceAddress const &key) override;
  Document GetOrCreate(ResourceAddress const &key) override;
  void     Set(ResourceAddress const &key, StateValue const &value) override;
  bool     Lock(ShardIndex shard) override;
  bool     Unlock(ShardIndex shard) override;
  Keys     KeyDump() const override;
  void     Reset() override;
  /// @}

  // Operators
  StateQuery &operator=(StateQuery const &) = delete;
  StateQuery &operator=(StateQuery &&) = delete;

private:
  using RpcClient = fetch::muddle::rpc::Client;
  using MuddleAddress = fetch::byte_array::ConstByteArray;

  MuddleAddress LookupAddress() const;

  MuddleEndpoint &  endpoint_;
  mutable RpcClient client_;
};
