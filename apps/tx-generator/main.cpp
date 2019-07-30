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
#include "core/commandline/params.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_builder.hpp"
#include "ledger/chain/transaction_serializer.hpp"
#include "storage/resource_mapper.hpp"
#include "vectorise/threading/pool.hpp"
#include "settings/detail/csv_string_helpers.hpp"

#include <chrono>
#include <fstream>
#include <memory>
#include <random>
#include <vector>
#include <sstream>

using fetch::commandline::Params;
using fetch::crypto::ECDSASigner;
using fetch::threading::Pool;
using fetch::byte_array::ConstByteArray;
using fetch::ledger::TransactionBuilder;
using fetch::ledger::TransactionSerializer;
using fetch::ledger::Address;
using fetch::serializers::MsgPackSerializer;
using fetch::serializers::SizeCounter;
using fetch::BitVector;
using fetch::storage::ResourceAddress;

using KeyPtr       = std::unique_ptr<ECDSASigner>;
using KeyArray     = std::vector<KeyPtr>;
using AddressPtr   = std::unique_ptr<Address>;
using AddressArray = std::vector<AddressPtr>;
using Clock        = std::chrono::high_resolution_clock;
using EncodedTxs   = std::vector<ConstByteArray>;

static std::size_t RoundUp(std::size_t value, std::size_t divisor)
{
  return (value + (divisor - 1)) / divisor;
}

static uint32_t DetermineShardIndex(Address const &address, uint32_t log2_shards)
{
  ResourceAddress const resource_address{"fetch.token.state." + address.display()};
  return resource_address.lane(log2_shards);
}

static ConstByteArray EncodeWealthTx(ECDSASigner const &from, Address const &from_address,
                                     uint32_t log2_shards, uint64_t amount)
{
  uint32_t num_shards = 1u << log2_shards;

  // generate the correct shard mask
  BitVector vector{num_shards};
  vector.set(DetermineShardIndex(from_address, log2_shards), 1);  // set the lane for the tx

  // form the transaction
  TransactionBuilder builder{};
  builder.ValidUntil(1000000u);
  builder.ChargeLimit(10u);
  builder.ChargeRate(2u);
  builder.TargetChainCode("fetch.token", vector);
  builder.Action("wealth");
  builder.Data(R"({"amount": )" + std::to_string(amount) + "}");
  builder.Signer(from.identity());
  builder.From(from_address);

  // finalise the transaction
  auto tx = builder.Seal().Sign(from).Build();

  // serialize the transaction
  TransactionSerializer serializer{};
  serializer.Serialize(*tx);

  return serializer.data();
}

static ConstByteArray EncodeTransferTx(ECDSASigner const &from, Address const &from_address,
                                       Address const &to, uint64_t amount)
{
  TransactionBuilder builder{};
  builder.From(from_address);
  builder.ValidUntil(1000000u);
  builder.ChargeLimit(5u);
  builder.ChargeRate(1u);
  builder.Transfer(to, amount);
  builder.Signer(from.identity());

  auto tx = builder.Seal().Sign(from).Build();

  TransactionSerializer serializer{};
  serializer.Serialize(*tx);

  return serializer.data();
}

static EncodedTxs CreateWealthTx(std::size_t count)
{
  Pool pool{};

  // allocate up front the vector
  std::vector<ConstByteArray> encoded_txs(count);

  // create all the transactions
  std::size_t const num_tx_per_thread = (count + pool.concurrency() - 1) / pool.concurrency();
  for (std::size_t i = 0; i < pool.concurrency(); ++i)
  {
    // compute the range of data to be populated
    std::size_t const start = num_tx_per_thread * i;
    std::size_t const end   = std::min(count, start + num_tx_per_thread);

    pool.Dispatch([start, end, &encoded_txs]() {
      static constexpr std::size_t LOG2_VECTOR_SIZE = 7u;

      for (std::size_t j = start; j < end; ++j)
      {
        // generate a new key for this transaction
        ECDSASigner from;
        Address     from_address{from.identity()};

        encoded_txs[j] = EncodeWealthTx(from, from_address, LOG2_VECTOR_SIZE, 1000);
      }
    });
  }

  pool.Wait();

  return encoded_txs;
}

static EncodedTxs CreateTransferTx(std::size_t count, uint32_t log2_lanes, EncodedTxs &wealth_txs)
{
  static constexpr std::size_t NUM_KEYS = 2;

  Pool pool{8};

  std::vector<ConstByteArray> encoded_tx(count);
  uint32_t                    num_lanes = 1u << log2_lanes;

  std::vector<KeyArray>     transfer_list(num_lanes);
  std::vector<AddressArray> address_list(num_lanes);

  // Step 1. Generate a set of keys that exist on the same shard
  for (uint32_t i = 0; i < num_lanes; ++i)
  {
    pool.Dispatch([i, log2_lanes, &transfer_list, &address_list]() {
      KeyArray     keys;
      AddressArray addresses;

      for (std::size_t j = 0; j < NUM_KEYS; ++j)
      {
        // search loop
        for (;;)
        {
          // generate a key and an associated address
          auto generated_key     = std::make_unique<ECDSASigner>();
          auto generated_address = std::make_unique<Address>(generated_key->identity());

          if (i == DetermineShardIndex(*generated_address, log2_lanes))
          {
            keys.emplace_back(std::move(generated_key));
            addresses.emplace_back(std::move(generated_address));
            break;
          }
        }
      }

      // return the generated keys and addresses
      transfer_list[i] = std::move(keys);
      address_list[i]  = std::move(addresses);
    });
  }

  // wait until all the address creation has been completed
  pool.Wait();

  // determine the number of batches to use
  std::size_t num_batches = 1;
  if (num_lanes < pool.concurrency())
  {
    num_batches = pool.concurrency() / num_lanes;
  }

  // Step 2a. Generate the create wealth transaction set
  for (uint32_t lane = 0; lane < num_lanes; ++lane)
  {
    wealth_txs.emplace_back(EncodeWealthTx(
        *transfer_list.at(lane).at(0), *address_list.at(lane).at(0), log2_lanes, 1000000000000));
  }

  // Step 2b. Generate the set of transactions
  std::size_t const batch_size = RoundUp(count, num_batches * num_lanes);

  for (uint32_t lane = 0; lane < num_lanes; ++lane)
  {
    for (std::size_t batch_idx = 0; batch_idx < num_batches; ++batch_idx)
    {
      pool.Dispatch([&encoded_tx, &transfer_list, &address_list, lane, num_lanes, batch_idx,
                     batch_size, count]() {
        KeyArray const &    keys      = transfer_list[lane];
        AddressArray const &addresses = address_list[lane];

        for (std::size_t i = 0; i < batch_size; ++i)
        {
          // calculate the destination index in the vector
          std::size_t const output_idx =
              (((batch_idx * batch_size) + i) * num_lanes) + lane;

          if (output_idx < count)
          {
            encoded_tx[output_idx] =
                EncodeTransferTx(*keys.at(0), *addresses.at(0), *addresses.at(1), 1u);
          }
        }
      });
    }
  }

  // wait for the generation to complete
  pool.Wait();

  return encoded_tx;
}

static std::size_t Flush(std::string const &filename, EncodedTxs const &encoded_txs)
{
  // determine the size
  SizeCounter<ByteArrayBuffer> counter{};
  counter << encoded_txs;

  ByteArrayBuffer buffer{};
  buffer.Reserve(counter.size());  // pre-allocate
  buffer << encoded_txs;

  // flush the stream
  std::ofstream output_stream{filename.c_str(), std::ios::out | std::ios::binary};
  output_stream.write(buffer.data().char_pointer(),
                      static_cast<std::streamsize>(buffer.data().size()));
  output_stream.close();

  return counter.size();
}

int main(int argc, char **argv)
{
  std::string              raw_counts;
  std::vector<std::size_t> counts{};
  int32_t                  log2_lanes{-1};
  std::string              output{};

  // build the parser
  Params parser{};
  parser.add(raw_counts, "count", "The number of tx to generate");
  parser.add(log2_lanes, "log2-lanes", "The log2 number of lanes to build transactions for", -1);
  parser.add(output, "prefix", "The file being generated", std::string{"out"});

  // parse the command line
  parser.Parse(argc, argv);

  std::istringstream iss{raw_counts};
  fetch::settings::detail::FromCommaSeparatedList(iss, counts);

  // sort the counts in reverse order
  std::sort(counts.begin(), counts.end(), [](std::size_t a, std::size_t b) {
    return a > b;
  });

  // create all the keys
  EncodedTxs encoded_txs{};
  EncodedTxs setup_txs{};

  std::cout << "Generating tx..." << std::endl;

  auto const started = Clock::now();

  // make the transactions
  if (log2_lanes >= 0)
  {
    encoded_txs = CreateTransferTx(counts[0], static_cast<uint32_t>(log2_lanes), setup_txs);
  }
  else
  {
    encoded_txs = CreateWealthTx(counts[0]);
  }

  auto const stopped = Clock::now();

  auto const delta_ns = (stopped - started).count();
  auto const tx_rate =
      (static_cast<double>(encoded_txs.size()) * 1e9) / static_cast<double>(delta_ns);
  std::cout << "Generating tx...complete (tx rate: " << tx_rate
            << " total: " << static_cast<double>(delta_ns) / 1e9 << ")" << std::endl;

  // loop through all the required counts
  for (auto const count : counts)
  {
    std::ostringstream oss;
    oss << output << '_' << count << ".bin";
    std::string const output_filename{oss.str()};

    std::cout << "Generating contents... (" << output_filename << ')' << std::endl;

    // update the size of the array
    assert(count > encoded_txs.size());
    encoded_txs.resize(count); // reduce the size of the array

    auto const counter_size = Flush(output_filename, encoded_txs);

    std::cout << "Generating contents...complete (length: " << counter_size << ')' << std::endl;
  }

  // flush the setup transaction (if they exist)
  if (!setup_txs.empty())
  {
    std::ostringstream oss;
    oss << output << '_' << "setup.bin";

    Flush(oss.str(), setup_txs);
  }

  return EXIT_SUCCESS;
}
