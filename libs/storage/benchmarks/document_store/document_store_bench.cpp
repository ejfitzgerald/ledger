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

#include "core/byte_array/byte_array.hpp"
#include "storage/resource_mapper.hpp"
#include "storage/new_revertible_document_store.hpp"

#include "benchmark/benchmark.h"

#include <string>
#include <iostream>

namespace {

using fetch::storage::NewRevertibleDocumentStore;
using fetch::storage::ResourceID;
using fetch::storage::ResourceAddress;
using fetch::byte_array::ByteArray;

using Resources = std::vector<ResourceAddress>;

void DocumentStoreRandomGets(benchmark::State &state)
{
  NewRevertibleDocumentStore store;
  store.New("state.db", "state.index.db", "index.db", "history.db", true);

  // create all the sets to be made
  Resources resources{};
  resources.reserve(state.max_iterations);
  for (std::size_t i = 0; i < state.max_iterations; ++i)
  {
    resources.emplace_back(ResourceAddress{std::to_string(state.max_iterations) + "---" + std::to_string(i)});
  }

  ByteArray data;
  data.Resize(8);
  for (auto it = resources.crbegin(), end = resources.crend(); it != end; ++it)
  {
    store.Set(*it, data);
  }

  std::size_t idx{0};
  for (auto _ : state)
  {
    store.Get(resources[idx]);
    ++idx;
  }
}

void DocumentStoreRandomSets(benchmark::State &state)
{
  NewRevertibleDocumentStore store;
  store.New("state.db", "state.index.db", "index.db", "history.db", true);

  // create all the sets to be made
  Resources resources{};
  resources.reserve(state.max_iterations);
  for (std::size_t i = 0; i < state.max_iterations; ++i)
  {
    resources.emplace_back(ResourceAddress{std::to_string(state.max_iterations) + "---" + std::to_string(i)});
  }

  ByteArray data;
  data.Resize(8);

  std::size_t idx{0};
  for (auto _ : state)
  {
    store.Set(resources[idx], data);
    ++idx;
  }
}

} // namespace

BENCHMARK(DocumentStoreRandomGets);
BENCHMARK(DocumentStoreRandomSets);

BENCHMARK_MAIN();
