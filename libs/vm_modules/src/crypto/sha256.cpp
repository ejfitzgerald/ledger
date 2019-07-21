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

#include "vm/common.hpp"
#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/crypto/sha256.hpp"
#include "vm_modules/math/bignumber.hpp"

#include <utility>

namespace fetch {

using namespace vm;

namespace vm_modules {

SHA256Wrapper::SHA256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  : fetch::vm::Object(vm, type_id)
{}

fetch::vm::Ptr<SHA256Wrapper> SHA256Wrapper::Constructor(fetch::vm::VM *   vm,
                                                         fetch::vm::TypeId type_id)
{
  return new SHA256Wrapper(vm, type_id);
}

void SHA256Wrapper::Bind(vm::Module &module)
{
  auto const sha256_ctor_estimator = ConstantEstimator<0>::Get();

  module.CreateClassType<SHA256Wrapper>("SHA256")
      .CreateConstuctor<decltype(sha256_ctor_estimator)>(std::move(sha256_ctor_estimator))
      .CreateMemberFunction("update", &SHA256Wrapper::UpdateUInt256, ConstantEstimator<1>::Get())
      .CreateMemberFunction("update", &SHA256Wrapper::UpdateString, ConstantEstimator<1>::Get())
      .CreateMemberFunction("update", &SHA256Wrapper::UpdateBuffer, ConstantEstimator<1>::Get())
      .CreateMemberFunction("final", &SHA256Wrapper::Final, ConstantEstimator<0>::Get())
      .CreateMemberFunction("reset", &SHA256Wrapper::Reset, ConstantEstimator<0>::Get());
}

void SHA256Wrapper::UpdateUInt256(vm::Ptr<vm_modules::math::UInt256Wrapper> const &uint)
{
  hasher_.Update(uint->number().pointer(), uint->number().size());
}

void SHA256Wrapper::UpdateString(vm::Ptr<vm::String> const &str)
{
  hasher_.Update(str->str);
}

void SHA256Wrapper::UpdateBuffer(Ptr<ByteArrayWrapper> const &buffer)
{
  hasher_.Update(buffer->byte_array());
}

void SHA256Wrapper::Reset()
{
  hasher_.Reset();
}

Ptr<math::UInt256Wrapper> SHA256Wrapper::Final()
{
  return vm_->CreateNewObject<math::UInt256Wrapper>(hasher_.Final());
}

}  // namespace vm_modules
}  // namespace fetch
