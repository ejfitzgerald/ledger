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

#include "vm/vm.hpp"

#include <cstddef>

namespace fetch {
namespace vm {

template <std::size_t NUM_PARAMS>
struct ConstantEstimator;

template <>
struct ConstantEstimator<0>
{
  static decltype(auto) Get(VM::ChargeAmount const charge = 1u)
  {
    return [charge](VM *) -> VM::ChargeAmount { return charge; };
  }
};

template <>
struct ConstantEstimator<1>
{
  static decltype(auto) Get(VM::ChargeAmount const charge = 1u)
  {
    return [charge](VM *, auto const &) -> VM::ChargeAmount { return charge; };
  }
};

template <>
struct ConstantEstimator<2>
{
  static decltype(auto) Get(VM::ChargeAmount const charge = 1u)
  {
    return [charge](VM *, auto const &, auto const &) -> VM::ChargeAmount { return charge; };
  }
};

template <>
struct ConstantEstimator<3>
{
  static decltype(auto) Get(VM::ChargeAmount const charge = 1u)
  {
    return [charge](VM *, auto const &, auto const &, auto const &) -> VM::ChargeAmount {
      return charge;
    };
  }
};

template <>
struct ConstantEstimator<4>
{
  static decltype(auto) Get(VM::ChargeAmount const charge = 1u)
  {
    return [charge](VM *, auto const &, auto const &, auto const &,
                    auto const &) -> VM::ChargeAmount { return charge; };
  }
};

template <>
struct ConstantEstimator<5>
{
  static decltype(auto) Get(VM::ChargeAmount const charge = 1u)
  {
    return [charge](VM *, auto const &, auto const &, auto const &, auto const &,
                    auto const &) -> VM::ChargeAmount { return charge; };
  }
};

template <>
struct ConstantEstimator<6>
{
  static decltype(auto) Get(VM::ChargeAmount const charge = 1u)
  {
    return [charge](VM *, auto const &, auto const &, auto const &, auto const &, auto const &,
                    auto const &) -> VM::ChargeAmount { return charge; };
  }
};

/**
 * @tparam Estimator
 * @tparam Ts
 * @param vm
 * @param e charge estimator function. Should take the same parameters as the opcode handler, and
 * return a VM::ChargeAmount
 * @param parameters
 * @return false if executing the opcode would exceed the specified charge limit; true otherwise.
 */
template <typename Estimator, typename... Ts>
bool EstimateCharge(VM *const vm, Estimator &&e, Ts const &... parameters)
{
  auto const charge_estimate = e(vm, parameters...);

  vm->IncreaseChargeTotal(charge_estimate);

  if (vm->GetChargeTotal() > vm->GetChargeLimit())
  {
    vm->RuntimeError("Charge limit exceeded");

    return false;
  }

  return true;
}

}  // namespace vm
}  // namespace fetch
