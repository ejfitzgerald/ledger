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
#include "vm/common.hpp"
#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

using fetch::vm::ConstantEstimator;

namespace fetch {
namespace vm_modules {

ByteArrayWrapper::ByteArrayWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                   byte_array::ByteArray bytearray)
  : fetch::vm::Object(vm, type_id)
  , byte_array_(std::move(bytearray))
{}

fetch::vm::Ptr<ByteArrayWrapper> ByteArrayWrapper::Constructor(fetch::vm::VM *   vm,
                                                               fetch::vm::TypeId type_id, int32_t n)
{
  return new ByteArrayWrapper(vm, type_id, byte_array::ByteArray(std::size_t(n)));
}

fetch::vm::Ptr<ByteArrayWrapper> ByteArrayWrapper::Constructor(
    fetch::vm::VM *vm, fetch::vm::TypeId type_id, byte_array::ByteArray const &bytearray)
{
  return new ByteArrayWrapper(vm, type_id, bytearray);
}

void ByteArrayWrapper::Bind(vm::Module &module)
{
  auto const byte_array_wrapper_ctor_estimator = ConstantEstimator<1>::Get();
  module.CreateClassType<ByteArrayWrapper>("Buffer")
      .CreateConstuctor<decltype(byte_array_wrapper_ctor_estimator), int32_t>(
          std::move(byte_array_wrapper_ctor_estimator))
      .CreateMemberFunction("copy", &ByteArrayWrapper::Copy, ConstantEstimator<0>::Get());
}

fetch::vm::Ptr<ByteArrayWrapper> ByteArrayWrapper::Copy() const
{
  return vm_->CreateNewObject<ByteArrayWrapper>(byte_array_.Copy());
}

byte_array::ByteArray ByteArrayWrapper::byte_array() const
{
  return byte_array_;
}

bool ByteArrayWrapper::SerializeTo(vm::ByteArrayBuffer &buffer)
{
  buffer << byte_array_;

  return true;
}

bool ByteArrayWrapper::DeserializeFrom(vm::ByteArrayBuffer &buffer)
{
  buffer >> byte_array_;

  return true;
}

}  // namespace vm_modules
}  // namespace fetch
