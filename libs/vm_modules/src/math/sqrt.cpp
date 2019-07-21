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

#include "math/standard_functions/sqrt.hpp"
#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm_modules/math/sqrt.hpp"

using fetch::vm::ConstantEstimator;

namespace fetch {
namespace vm_modules {
namespace math {

namespace {

template <typename T>
fetch::math::meta::IfIsMath<T, T> Sqrt(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::Sqrt(a, x);
  return x;
}

}  // namespace

void BindSqrt(fetch::vm::Module &module)
{
  module.CreateFreeFunction<float_t>("sqrt", &Sqrt<float_t>, ConstantEstimator<1>::Get());
  module.CreateFreeFunction<double_t>("sqrt", &Sqrt<double_t>, ConstantEstimator<1>::Get());
  module.CreateFreeFunction<fixed_point::fp32_t>("sqrt", &Sqrt<fixed_point::fp32_t>,
                                                 ConstantEstimator<1>::Get());
  module.CreateFreeFunction<fixed_point::fp64_t>("sqrt", &Sqrt<fixed_point::fp64_t>,
                                                 ConstantEstimator<1>::Get());
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
