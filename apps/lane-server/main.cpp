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

#include "constellation/muddle_status_http_module.hpp"
#include "constellation/telemetry_http_module.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/filesystem/read_file_contents.hpp"
#include "crypto/ecdsa.hpp"
#include "http/http_server.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "logging/logging.hpp"
#include "muddle/network_id.hpp"
#include "network/management/network_manager.hpp"

#include <chrono>
#include <csignal>
#include <fstream>
#include <memory>
#include <thread>
#include <vector>

namespace {

using fetch::ledger::LaneService;
using fetch::ledger::ShardConfig;
using fetch::muddle::NetworkId;
using fetch::crypto::ECDSASigner;
using fetch::commandline::ParamsParser;
using fetch::network::NetworkManager;
using fetch::http::HTTPServer;
using fetch::http::HTTPModule;

using LaneServicePtr = std::unique_ptr<LaneService>;
using HTTPServerPtr  = std::unique_ptr<HTTPServer>;
using HTTPModulePtr  = std::unique_ptr<HTTPModule>;

constexpr char const *LOGGING_NAME = "main";

std::atomic<bool>        global_running{true};
std::atomic<bool>        global_ready{false};
std::atomic<std::size_t> global_interrupt_count{0};

struct Settings
{
  uint16_t http_port{0};
};

struct HttpServerContext
{
  HTTPServerPtr              server{};
  std::vector<HTTPModulePtr> modules{};
};

class HealthCheckModule : public fetch::http::HTTPModule
{
public:
  HealthCheckModule()
  {
    Get("/api/health/alive", "Endpoint to check if the server is alive.",
        [](fetch::http::ViewParameters const &, fetch::http::HTTPRequest const &) {
          return fetch::http::CreateJsonResponse("{}");
        });

    Get("/api/health/ready", "Endpoint to check if the server is ready.",
        [](fetch::http::ViewParameters const &, fetch::http::HTTPRequest const &) {
          auto const status = (global_ready) ? fetch::http::Status::SUCCESS_OK
                                             : fetch::http::Status::CLIENT_ERROR_BAD_REQUEST;

          return fetch::http::CreateJsonResponse("{}", status);
        });
  }
};

ShardConfig::CertificatePtr LoadOrCreateCertificate(char const *filename)
{
  // read the contents of the certificate file
  auto contents = fetch::core::ReadContentsOfFile(filename);

  // try and load the certificate
  if (!contents.empty())
  {
    return std::make_shared<ECDSASigner>(contents);
  }

  // create a new key
  auto        certificate = std::make_shared<ECDSASigner>();
  auto        private_key = certificate->private_key();
  auto const &private_key_ref{private_key};

  // flush to disk
  std::ofstream stream{filename, std::ios::out | std::ios::binary | std::ios::trunc};
  stream.write(private_key_ref.char_pointer(),
               static_cast<std::streamsize>(private_key_ref.size()));

  return certificate;
}

void BuildConfiguration(int argc, char const *const *argv, ShardConfig &cfg, Settings &settings)
{
  // parse the command line
  ParamsParser parser;
  parser.Parse(argc, argv);

  cfg.lane_id      = parser.GetParam<uint32_t>("lane", 0);
  cfg.num_lanes    = parser.GetParam<uint32_t>("num-lanes", 1);
  cfg.storage_path = "";

  cfg.external_name       = parser.GetParam<std::string>("external", "127.0.0.1");
  cfg.external_identity   = LoadOrCreateCertificate("external.key");
  cfg.external_port       = parser.GetParam<uint16_t>("external-port", 8010);
  cfg.external_network_id = NetworkId{(cfg.lane_id & 0xFFFFFFu) | (uint32_t{'L'} << 24u)};

  cfg.internal_name       = parser.GetParam<std::string>("external", "127.0.0.1");
  cfg.internal_identity   = LoadOrCreateCertificate("internal.key");
  cfg.internal_port       = parser.GetParam<uint16_t>("internal-port", 8011);
  cfg.internal_network_id = NetworkId{"ISRD"};

  settings.http_port = parser.GetParam<uint16_t>("http-port", 0);
}

HttpServerContext CreateHttpServer(Settings const &settings, NetworkManager const &nm)
{
  HttpServerContext ctx;

  if (settings.http_port > 0)
  {
    // create all the modules
    ctx.modules.reserve(3);
    ctx.modules.emplace_back(std::make_unique<fetch::constellation::MuddleStatusModule>());
    ctx.modules.emplace_back(std::make_unique<fetch::TelemetryHttpModule>());
    ctx.modules.emplace_back(std::make_unique<HealthCheckModule>());

    // create the server
    ctx.server = std::make_unique<HTTPServer>(nm);

    // add all the modules to the server
    for (auto const &mod : ctx.modules)
    {
      ctx.server->AddModule(*mod);
    }

    // start the http server
    ctx.server->Start(settings.http_port);
  }

  return ctx;
}

void InterruptHandler(int /*signal*/)
{
  std::size_t const interrupt_count = ++global_interrupt_count;

  if (interrupt_count > 1)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "User requests stop of service (count: ", interrupt_count, ")");
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "User requests stop of service");
  }

  // signal the process should stop
  global_running = false;

  if (interrupt_count >= 3)
  {
    std::exit(1);
  }
}

int Run(int argc, char const *const *argv)
{
  // build the configuration
  ShardConfig cfg{};
  Settings    settings{};
  BuildConfiguration(argc, argv, cfg, settings);

  // create the network manager
  NetworkManager nm{"lane", 1};
  nm.Start();

  // create the http stack if required
  auto http = CreateHttpServer(settings, nm);

  // create the lane service
  auto lane_service =
      std::make_unique<LaneService>(nm, std::move(cfg), LaneService::Mode::LOAD_DATABASE);

  // start the internal network
  lane_service->StartInternal();

  // wait for the process to stop
  while (global_running)
  {
    global_ready = true; // signal that the server is up and ready
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

  lane_service->StopInternal();

  if (http.server)
  {
    http.server->Stop();
  }

  nm.Stop();

  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char const *const *argv)
{
  int exit_code{EXIT_FAILURE};

  try
  {
    std::signal(SIGINT, InterruptHandler);
    std::signal(SIGTERM, InterruptHandler);

    exit_code = Run(argc, argv);
  }
  catch (std::exception const &ex)
  {
    std::cout << "Fatal Error: " << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cout << "Fatal Error: Unknown Error" << std::endl;
  }

  return exit_code;
}
