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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/module.hpp"

#include <cstdint>

namespace fetch {
namespace vm_modules {

/**
 * method for type converting from arithmetic to string
 */
template <typename T>
fetch::math::meta::IfIsNonFixedPointArithmetic<T, fetch::vm::Ptr<fetch::vm::String>> ToString(
    fetch::vm::VM *vm, T const &a)
{
  if (std::is_same<T, bool>::value)
  {
    fetch::vm::Ptr<fetch::vm::String> ret(
        new fetch::vm::String(vm, static_cast<bool>(a) ? "true" : "false"));
    return ret;
  }
  else
  {
    fetch::vm::Ptr<fetch::vm::String> ret(new fetch::vm::String(vm, std::to_string(a)));
    return ret;
  }
}

template <typename T>
fetch::math::meta::IfIsFixedPoint<T, fetch::vm::Ptr<fetch::vm::String>> ToString(fetch::vm::VM *vm,
                                                                                 T const &      a)
{
  std::stringstream stream;
  stream << a;
  fetch::vm::Ptr<fetch::vm::String> ret(new fetch::vm::String(vm, stream.str()));
  return ret;
}

template <typename T>
bool ToBool(fetch::vm::VM * /*vm*/, T const &a)
{
  return static_cast<bool>(a);
}

static auto const type_convert_estimator =
    [](fetch::vm::VM *, auto const &) -> fetch::vm::VM::ChargeAmount { return 1u; };

inline void CreateToString(fetch::vm::Module &module)
{
  module.CreateFreeFunction("toString", &ToString<int8_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<uint8_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<int16_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<uint16_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<int32_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<uint32_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<int64_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<uint64_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<float_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<double_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<bool>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<fixed_point::fp32_t>, type_convert_estimator);
  module.CreateFreeFunction("toString", &ToString<fixed_point::fp64_t>, type_convert_estimator);
}

inline void CreateToBool(fetch::vm::Module &module)
{
  module.CreateFreeFunction("toBool", &ToBool<int32_t>, type_convert_estimator);
  module.CreateFreeFunction("toBool", &ToBool<uint32_t>, type_convert_estimator);
  module.CreateFreeFunction("toBool", &ToBool<int64_t>, type_convert_estimator);
  module.CreateFreeFunction("toBool", &ToBool<uint64_t>, type_convert_estimator);
  module.CreateFreeFunction("toBool", &ToBool<float_t>, type_convert_estimator);
  module.CreateFreeFunction("toBool", &ToBool<double_t>, type_convert_estimator);
}

}  // namespace vm_modules
}  // namespace fetch
