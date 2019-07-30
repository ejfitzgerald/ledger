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

#include "core/service_ids.hpp"
#include "network/service/call_context.hpp"

#include "storage/document_store_protocol.hpp"

using fetch::ledger::StorageInterface;
using fetch::storage::RevertibleDocumentStoreProtocol;

using Document = StorageInterface::Document;
using Keys     = StorageInterface::Keys;

StateQuery::StateQuery(MuddleEndpoint &endpoint)
  : endpoint_{endpoint}
  , client_{"client", endpoint_, fetch::SERVICE_LANE_CTRL, fetch::CHANNEL_RPC}
{}

Document StateQuery::Get(ResourceAddress const &key)
{
  auto promise =
      client_.CallSpecificAddress(LookupAddress(), fetch::RPC_STATE,
                                  RevertibleDocumentStoreProtocol::GET, key.as_resource_id());
  return promise->As<Document>();
}

Document StateQuery::GetOrCreate(ResourceAddress const &key)
{
  auto promise = client_.CallSpecificAddress(LookupAddress(), fetch::RPC_STATE,
                                             RevertibleDocumentStoreProtocol::GET_OR_CREATE,
                                             key.as_resource_id());
  return promise->As<Document>();
}

void StateQuery::Set(ResourceAddress const &key, StateValue const &value)
{
  auto promise = client_.CallSpecificAddress(LookupAddress(), fetch::RPC_STATE,
                                             RevertibleDocumentStoreProtocol::SET,
                                             key.as_resource_id(), value);
  promise->Wait();
}

bool StateQuery::Lock(ShardIndex shard)
{
  FETCH_UNUSED(shard);
  return false;
}

bool StateQuery::Unlock(ShardIndex shard)
{
  FETCH_UNUSED(shard);
  return false;
}

Keys StateQuery::KeyDump() const
{
  auto promise = client_.CallSpecificAddress(LookupAddress(), fetch::RPC_STATE,
                                             RevertibleDocumentStoreProtocol::KEY_DUMP);
  return promise->As<Keys>();
}

void StateQuery::Reset()
{
  auto promise = client_.CallSpecificAddress(LookupAddress(), fetch::RPC_STATE,
                                             RevertibleDocumentStoreProtocol::RESET);
  promise->Wait();
}

StateQuery::MuddleAddress StateQuery::LookupAddress() const
{
  auto const peers = endpoint_.GetDirectlyConnectedPeers();

  if (peers.size() != 1)
  {
    throw std::runtime_error("Unable to determine connected peer address");
  }

  return peers[0];
}
