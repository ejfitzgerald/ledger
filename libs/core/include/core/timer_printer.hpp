#pragma once
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

#include "moment/clocks.hpp"

namespace fetch {
namespace core {

namespace
{
uint64_t GetTimeHelper()
{
  return GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM), moment::TimeAccuracy::MILLISECONDS);
}
}

class TimerPrinter
{
public:
  void Start(std::string const &action)
  {
    uint64_t start_time = GetTimeHelper();
    action_start_[action] = start_time;
  }

  void Stop(std::string const &action)
  {
    uint64_t stop_time = GetTimeHelper();
    action_stop_[action] = stop_time;
  }

  void Reset()
  {
    action_start_.clear();
    action_stop_.clear();
  }

  void Print()
  {
    for(auto const &i : action_start_)
    {
      FETCH_LOG_INFO("TimerPrinter", "Action: ", i.first, " time taken: ", action_stop_[i.first] - action_start_[i.first], " at: ", action_start_[i.first]);
    }
  }

private:
  std::map<std::string, uint64_t> action_start_;
  std::map<std::string, uint64_t> action_stop_;
};


}  // namespace core
}  // namespace fetch
