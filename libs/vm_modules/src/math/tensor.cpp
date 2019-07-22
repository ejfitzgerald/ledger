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

#include "math/tensor.hpp"
#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/tensor.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace vm_modules {
namespace math {

VMTensor::VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                   std::vector<std::uint64_t> const &shape)
  : fetch::vm::Object(vm, type_id)
  , tensor_(shape)
{}

VMTensor::VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, ArrayType tensor)
  : fetch::vm::Object(vm, type_id)
  , tensor_(std::move(tensor))
{}

VMTensor::VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  : fetch::vm::Object(vm, type_id)
  , tensor_{}
{}

fetch::vm::Ptr<VMTensor> VMTensor::Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                               fetch::vm::Ptr<fetch::vm::Array<SizeType>> shape)
{
  return {new VMTensor(vm, type_id, shape->elements)};
}

fetch::vm::Ptr<VMTensor> VMTensor::Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
{
  return {new VMTensor(vm, type_id)};
}

void VMTensor::Bind(fetch::vm::Module &module)
{
  auto const tensor_ctor_estimator = 1;

  module.CreateClassType<VMTensor>("Tensor")
      .CreateConstuctor<fetch::vm::Ptr<fetch::vm::Array<VMTensor::SizeType>>>(
          std::move(tensor_ctor_estimator))
      .CreateSerializeDefaultConstuctor<>()
      .CreateMemberFunction("at", &VMTensor::AtOne, 1)
      .CreateMemberFunction("at", &VMTensor::AtTwo, 1)
      .CreateMemberFunction("at", &VMTensor::AtThree, 1)
      .CreateMemberFunction("setAt", &VMTensor::SetAt, 1)
      .CreateMemberFunction("fill", &VMTensor::Fill, 1)
      .CreateMemberFunction("reshape", &VMTensor::Reshape, 1)
      .CreateMemberFunction("toString", &VMTensor::ToString, 1);
}

VMTensor::SizeVector VMTensor::shape() const
{
  return tensor_.shape();
}

////////////////////////////////////
/// ACCESSING AND SETTING VALUES ///
////////////////////////////////////

VMTensor::DataType VMTensor::AtOne(uint64_t idx1) const
{
  return tensor_.At(idx1);
}

VMTensor::DataType VMTensor::AtTwo(uint64_t idx1, uint64_t idx2) const
{
  return tensor_.At(idx1, idx2);
}

VMTensor::DataType VMTensor::AtThree(uint64_t idx1, uint64_t idx2, uint64_t idx3) const
{
  return tensor_.At(idx1, idx2, idx3);
}

void VMTensor::SetAt(uint64_t index, DataType value)
{
  tensor_.At(index) = value;
}

void VMTensor::Copy(ArrayType const &other)
{
  tensor_.Copy(other);
}

void VMTensor::Fill(DataType const &value)
{
  tensor_.Fill(value);
}

bool VMTensor::Reshape(fetch::vm::Ptr<fetch::vm::Array<SizeType>> const &new_shape)
{
  return tensor_.Reshape(new_shape->elements);
}

//////////////////////////////
/// PRINTING AND EXPORTING ///
//////////////////////////////

fetch::vm::Ptr<fetch::vm::String> VMTensor::ToString() const
{
  return new fetch::vm::String(vm_, tensor_.ToString());
}

VMTensor::ArrayType &VMTensor::GetTensor()
{
  return tensor_;
}

bool VMTensor::SerializeTo(serializers::ByteArrayBuffer &buffer)
{
  buffer << tensor_;
  return true;
}

bool VMTensor::DeserializeFrom(serializers::ByteArrayBuffer &buffer)
{
  buffer >> tensor_;
  return true;
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
