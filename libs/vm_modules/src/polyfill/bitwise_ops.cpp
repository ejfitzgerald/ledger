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

#include "vm/module.hpp"
#include "vm_modules/polyfill/bitwise_ops.hpp"

#include <cstdint>

namespace fetch {

using namespace vm;

namespace vm_modules {
namespace {

template <typename T>
T And(fetch::vm::VM * /*vm*/, T x, T s)
{
  return T(x & s);
}

template <typename T>
T Or(fetch::vm::VM * /*vm*/, T x, T s)
{
  return T(x | s);
}

}  // namespace

void BindBitwiseOps(vm::Module &module)
{
  module.CreateFreeFunction("and", &And<int32_t>, ConstantEstimator<2>::Get());
  module.CreateFreeFunction("and", &And<int64_t>, ConstantEstimator<2>::Get());
  module.CreateFreeFunction("and", &And<uint32_t>, ConstantEstimator<2>::Get());
  module.CreateFreeFunction("and", &And<uint64_t>, ConstantEstimator<2>::Get());

  module.CreateFreeFunction("or", &Or<int32_t>, ConstantEstimator<2>::Get());
  module.CreateFreeFunction("or", &Or<int64_t>, ConstantEstimator<2>::Get());
  module.CreateFreeFunction("or", &Or<uint32_t>, ConstantEstimator<2>::Get());
  module.CreateFreeFunction("or", &Or<uint64_t>, ConstantEstimator<2>::Get());
}

}  // namespace vm_modules
}  // namespace fetch
