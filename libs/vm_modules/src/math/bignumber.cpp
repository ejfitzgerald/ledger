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

#include "core/byte_array/encoders.hpp"
#include "vectorise/uint/uint.hpp"
#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm/object.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/math/bignumber.hpp"

#include <algorithm>
#include <cstdint>

using fetch::vm::ConstantEstimator;
using fetch::vm::Ptr;

namespace fetch {
namespace vm_modules {
namespace math {

fetch::vm::Ptr<fetch::vm::String> UInt256Wrapper::ToString(fetch::vm::VM *            vm,
                                                           Ptr<UInt256Wrapper> const &n)
{
  byte_array::ByteArray ba(32);
  for (uint64_t i = 0; i < 32; ++i)
  {
    ba[i] = n->number_[i];
  }

  fetch::vm::Ptr<fetch::vm::String> ret(
      new fetch::vm::String(vm, static_cast<std::string>(ToHex(ba))));
  return ret;
}

void UInt256Wrapper::Bind(vm::Module &module)
{
  auto const bignumber_ctor_estimator1 = ConstantEstimator<1>::Get();
  auto const bignumber_ctor_estimator2 = ConstantEstimator<1>::Get();
  auto const bignumber_ctor_estimator3 = ConstantEstimator<1>::Get();

  module.CreateClassType<UInt256Wrapper>("UInt256")
      .CreateSerializeDefaultConstuctor<uint64_t>(static_cast<uint64_t>(0))
      .CreateConstuctor<decltype(bignumber_ctor_estimator1), uint64_t>(
          std::move(bignumber_ctor_estimator1))
      .CreateConstuctor<decltype(bignumber_ctor_estimator2), Ptr<vm::String>>(
          std::move(bignumber_ctor_estimator2))
      .CreateConstuctor<decltype(bignumber_ctor_estimator3), Ptr<ByteArrayWrapper>>(
          std::move(bignumber_ctor_estimator3))
      .EnableOperator(vm::Operator::Equal)
      .EnableOperator(vm::Operator::NotEqual)
      .EnableOperator(vm::Operator::LessThan)
      //        .EnableOperator(vm::Operator::LessThanOrEqual)
      .EnableOperator(vm::Operator::GreaterThan)
      //        .EnableOperator(vm::Operator::GreaterThanOrEqual)
      //        .CreateMemberFunction("toBuffer", &UInt256Wrapper::ToBuffer)
      .CreateMemberFunction("increase", &UInt256Wrapper::Increase, ConstantEstimator<0>::Get())
      //        .CreateMemberFunction("lessThan", &UInt256Wrapper::LessThan)
      .CreateMemberFunction("logValue", &UInt256Wrapper::LogValue, ConstantEstimator<0>::Get())
      .CreateMemberFunction("toFloat64", &UInt256Wrapper::ToFloat64, ConstantEstimator<0>::Get())
      .CreateMemberFunction("toInt32", &UInt256Wrapper::ToInt32, ConstantEstimator<0>::Get())
      .CreateMemberFunction("size", &UInt256Wrapper::size, ConstantEstimator<0>::Get());

  module.CreateFreeFunction("toString", &UInt256Wrapper::ToString, ConstantEstimator<1>::Get());
  module.CreateFreeFunction("toUInt64", &UInt256Wrapper::ToPrimitive<uint64_t>,
                            ConstantEstimator<1>::Get());
  module.CreateFreeFunction("toInt64", &UInt256Wrapper::ToPrimitive<int64_t>,
                            ConstantEstimator<1>::Get());
  module.CreateFreeFunction("toUInt32", &UInt256Wrapper::ToPrimitive<uint32_t>,
                            ConstantEstimator<1>::Get());
  module.CreateFreeFunction("toInt32", &UInt256Wrapper::ToPrimitive<int32_t>,
                            ConstantEstimator<1>::Get());
}

UInt256Wrapper::UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, UInt256 data)
  : fetch::vm::Object(vm, type_id)
  , number_(std::move(data))
{}

UInt256Wrapper::UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                               byte_array::ByteArray const &data)
  : fetch::vm::Object(vm, type_id)
  , number_(data)
{}

UInt256Wrapper::UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                               std::string const &data)
  : fetch::vm::Object(vm, type_id)
  , number_(data)
{}

UInt256Wrapper::UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, uint64_t data)
  : fetch::vm::Object(vm, type_id)
  , number_(data)
{}

fetch::vm::Ptr<UInt256Wrapper> UInt256Wrapper::Constructor(
    fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<ByteArrayWrapper> const &ba)
{
  try
  {
    return new UInt256Wrapper(vm, type_id, ba->byte_array());
  }
  catch (std::runtime_error const &e)
  {
    vm->RuntimeError(e.what());
  }
  return nullptr;
}

fetch::vm::Ptr<UInt256Wrapper> UInt256Wrapper::Constructor(fetch::vm::VM *   vm,
                                                           fetch::vm::TypeId type_id,
                                                           fetch::vm::Ptr<vm::String> const &ba)
{
  try
  {
    return new UInt256Wrapper(vm, type_id, ba->str);
  }
  catch (std::runtime_error const &e)
  {
    vm->RuntimeError(e.what());
  }
  return nullptr;
}

fetch::vm::Ptr<UInt256Wrapper> UInt256Wrapper::Constructor(fetch::vm::VM *   vm,
                                                           fetch::vm::TypeId type_id, uint64_t val)
{
  try
  {
    return new UInt256Wrapper(vm, type_id, val);
  }
  catch (std::runtime_error const &e)
  {
    vm->RuntimeError(e.what());
  }
  return nullptr;
}

double UInt256Wrapper::ToFloat64() const
{
  return ToDouble(number_);
}

int32_t UInt256Wrapper::ToInt32() const
{
  return static_cast<int32_t>(number_[0]);
}

double UInt256Wrapper::LogValue() const
{
  return Log(number_);
}

bool UInt256Wrapper::LessThan(Ptr<UInt256Wrapper> const &other) const
{
  return number_ < other->number_;
}

void UInt256Wrapper::Increase()
{
  ++number_;
}

UInt256Wrapper::SizeType UInt256Wrapper::size() const
{
  return number_.size();
}

vectorise::UInt<256> const &UInt256Wrapper::number() const
{
  return number_;
}

bool UInt256Wrapper::SerializeTo(serializers::ByteArrayBuffer &buffer)
{
  buffer << number_;
  return true;
}

bool UInt256Wrapper::DeserializeFrom(serializers::ByteArrayBuffer &buffer)
{
  buffer >> number_;
  return true;
}

bool UInt256Wrapper::ToJSON(vm::JSONVariant &variant)
{
  variant = vm::JSONVariant::Object();
  byte_array::ByteArray value(32);
  for (uint64_t i = 0; i < 32; ++i)
  {
    value[i] = number_[i];
  }

  variant["type"]  = GetUniqueId();
  variant["value"] = ToHex(value);
  return true;
}

bool UInt256Wrapper::FromJSON(vm::JSONVariant const &variant)
{
  if (!variant.IsObject())
  {
    vm_->RuntimeError("JSON deserialisation of " + GetUniqueId() + " must be an object.");
    return false;
  }

  if (!variant.Has("type"))
  {
    vm_->RuntimeError("JSON deserialisation of " + GetUniqueId() + " must have field 'type'.");
    return false;
  }

  if (!variant.Has("value"))
  {
    vm_->RuntimeError("JSON deserialisation of " + GetUniqueId() + " must have field 'value'.");
    return false;
  }

  if (variant["type"].As<std::string>() != GetUniqueId())
  {
    vm_->RuntimeError("Field 'type' must be '" + GetUniqueId() + "'.");
    return false;
  }

  if (!variant["value"].IsString())
  {
    vm_->RuntimeError("Field 'value' must be a hex encoded string.");
    return false;
  }

  // TODO(issue 1262): Caller can't unambiguously detect whether the conversion failed or not
  auto const value = FromHex(variant["value"].As<byte_array::ConstByteArray>());

  uint64_t i = 0;
  uint64_t n = std::min(32ul, value.size());
  for (; i < n; ++i)
  {
    number_[i] = value[i];
  }

  for (; i < 32; ++i)
  {
    number_[i] = 0;
  }

  return true;
}

bool UInt256Wrapper::IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return lhs->number_ == rhs->number_;
}

bool UInt256Wrapper::IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return lhs->number_ != rhs->number_;
}

bool UInt256Wrapper::IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return lhs->number_ < rhs->number_;
}

bool UInt256Wrapper::IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return rhs->number_ < lhs->number_;
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
