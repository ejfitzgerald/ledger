//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "settings.hpp"

#include "core/commandline/parameter_parser.hpp"
#include "logging/logging.hpp"
#include "vectorise/platform.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <thread>

namespace fetch {
namespace {

const uint32_t DEFAULT_NUM_LANES          = 1;
const uint32_t DEFAULT_NUM_SLICES         = 500;
const uint32_t DEFAULT_NUM_EXECUTORS      = DEFAULT_NUM_LANES;
const uint16_t DEFAULT_PORT               = 8000;
const uint16_t DEFAULT_MESSENGER_PORT     = 9010;
const uint32_t DEFAULT_BLOCK_INTERVAL     = 0;  // milliseconds - zero means no mining
const uint32_t DEFAULT_CABINET_SIZE       = 10;
const uint32_t DEFAULT_STAKE_DELAY_PERIOD = 5;
const uint32_t DEFAULT_AEON_PERIOD        = 25;
const uint32_t DEFAULT_MAX_PEERS          = 3;
const uint32_t DEFAULT_TRANSIENT_PEERS    = 1;
const uint32_t NUM_SYSTEM_THREADS = static_cast<uint32_t>(std::thread::hardware_concurrency());

class ArgvWrapper
{
public:
  explicit ArgvWrapper(Settings::UpdateBatch const &batch)
    : batch_(batch)
    , argc_((batch_.size() * 2) + 1)
    , argv_(argc_, nullptr)
  {
    argv_[0] = "wrapper";

    std::size_t idx{1};
    for (auto const &element : batch_)
    {
      assert(idx + 1 < argv_.size());

      argv_[idx]     = element.first.c_str();
      argv_[idx + 1] = element.second.c_str();

      idx += 2;
    }
  }

  int argc() const
  {
    return static_cast<int>(argc_);
  }

  char const *const *argv() const
  {
    return argv_.data();
  }

private:
  Settings::UpdateBatch const &batch_;
  std::size_t                  argc_{0};
  std::vector<char const *>    argv_;
};

}  // namespace

// clang-format off
/**
 * Construct the settings object
 */
Settings::Settings()
  : num_lanes             {*this, "lanes",                   DEFAULT_NUM_LANES,            "The number of lanes to be used"}
  , num_slices            {*this, "slices",                  DEFAULT_NUM_SLICES,           "The number of slices to be used"}
  , block_interval        {*this, "block-interval",          DEFAULT_BLOCK_INTERVAL,       "The block interval is milliseconds"}
  , external_lanes        {*this, "external-lanes",          false,                        "Signal that lane services are external to this process"}
  , standalone            {*this, "standalone",              false,                        "Signal the network should run in standalone mode"}
  , private_network       {*this, "private-network",         false,                        "Signal the network should run as part of a private network"}
  , db_prefix             {*this, "db-prefix",               "node_storage",               "The prefix for filenames related to constellation databases"}
  , port                  {*this, "port",                    DEFAULT_PORT,                 "The starting port for ledger services"}
  , peers                 {*this, "peers",                   {},                           "The comma separated list of addresses to initially connect to"}
  , external              {*this, "external",                "",                           "This node's global IP address or hostname"}
  , shard_prefix          {*this, "shard-prefix",            "",                           "This nodes shard address prefix"}
  , config                {*this, "config",                  "",                           "The path to the manifest configuration"}
  , max_peers             {*this, "max-peers",               DEFAULT_MAX_PEERS,            "The max number of peers to connect to"}
  , transient_peers       {*this, "transient-peers",         DEFAULT_TRANSIENT_PEERS,      "The number of the peers which will be random in answer sent to peer requests"}
  , peer_update_interval  {*this, "peers-update-cycle-ms",   0,                            "How fast to do peering updates"}
  , disable_signing       {*this, "disable-signing",         false,                        "Disable the signing of all network messages"}
  , kademlia_routing      {*this, "kademlia-routing",        true,                         "Controls if kademalia routing is used in the main P2P network"}
  , bootstrap             {*this, "bootstrap",               false,                        "Signal that we should connect to the bootstrap server"}
  , discoverable          {*this, "discoverable",            false,                        "Signal that this node can be advertised on the bootstrap server"}
  , hostname              {*this, "host-name",               "",                           "The hostname or identifier for this node"}
  , network_name          {*this, "network",                 "",                           "The name of the bootstrap network to connect to"}
  , token                 {*this, "token",                   "",                           "The authentication token when talking to bootstrap"}
  , num_processor_threads {*this, "processor-threads",       NUM_SYSTEM_THREADS,           "The number of processor threads"}
  , num_verifier_threads  {*this, "verifier-threads",        NUM_SYSTEM_THREADS,           "The number of verifier threads"}
  , num_executors         {*this, "executors",               DEFAULT_NUM_EXECUTORS,        "The number of transaction executors"}
  , genesis_file_location {*this, "genesis-file-location",   "",                           "Path to the genesis file (usually genesis_file.json)"}
  , experimental_features {*this, "experimental",            {},                           "The comma separated set of experimental features to enable"}
  , proof_of_stake        {*this, "pos",                     false,                        "Enable Proof of Stake consensus"}
  , max_cabinet_size      {*this, "max-cabinet-size",        DEFAULT_CABINET_SIZE,         "The maximum cabinet size"}
  , stake_delay_period    {*this, "stake-delay-period",      DEFAULT_STAKE_DELAY_PERIOD,   ""}
  , aeon_period           {*this, "aeon-period",             DEFAULT_AEON_PERIOD,          "The number of blocks one cabinet is governing"}
  , graceful_failure      {*this, "graceful-failure",        false,                        "Whether or not to shutdown on critical system failures"}
  , fault_tolerant        {*this, "fault-tolerant",          false,                        "Whether or not to allow critical system failures to cause a crash"}
  , enable_agents         {*this, "enable-agents",           false,                        "Run the node with agent support"}
  , messenger_port        {*this, "messenger-port",          DEFAULT_MESSENGER_PORT,       "Port that agents connect to"}
{}
// clang-format on

/**
 * Given the specified command line arguments, update the settings from the cmd line and the
 * environment
 *
 * @param argc The number of command line arguments
 * @param argv The array of command line arguments
 */
bool Settings::Update(int argc, char const *const *argv)
{
  UpdateFromEnv("CONSTELLATION_");
  UpdateFromArgs(argc, argv);
  return Validate();
}

bool Settings::Update(UpdateBatch const &batch)
{
  ArgvWrapper wrapper{batch};
  UpdateFromArgs(wrapper.argc(), wrapper.argv());

  return Validate();
}

/**
 * Display the summary of all the settings
 *
 * @param stream The output stream to populate
 * @param settings The reference to the settings object
 * @return The output stream that has been populated
 */
std::ostream &operator<<(std::ostream &stream, Settings const &settings)
{
  // First pass determine the max settings length
  std::size_t setting_name_length{0};
  for (auto const *setting : settings.settings())
  {
    setting_name_length = std::max(setting_name_length, setting->name().size());
  }

  // Send pass display the settings
  for (auto const *setting : settings.settings())
  {
    auto const &      name    = setting->name();
    std::size_t const padding = setting_name_length - name.size();

    // write name + padding
    stream << name;
    for (std::size_t i = 0; i < padding; ++i)
    {
      stream << '.';
    }
    stream << ": ";

    // write the value to the stream
    setting->ToStream(stream);
    stream << '\n';
  }

  return stream;
}

/**
 * Internal: Validate that all the parameters are correctly set
 *
 * @return true if the configuration is valid, otherwise false
 */
bool Settings::Validate()
{
  bool valid{true};

  // network mode checks
  if (standalone.value() && private_network.value())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Can not have both the -standalone and -private-network flags");
    valid = false;
  }

  if (external_lanes.value() && shard_prefix.value().empty())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Please specify the shard prefix when connecting to external lanes");
    valid = false;
  }

  // number of lanes check
  if (!platform::IsLog2(num_lanes.value()))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "The number of lanes needs to be a valid power of 2");
    valid = false;
  }

  return valid;
}

}  // namespace fetch
