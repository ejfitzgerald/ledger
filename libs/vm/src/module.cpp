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
#include "vm/common.hpp"
#include "vm/module.hpp"
#include "vm/sharded_state.hpp"
#include "vm/variant.hpp"
#include "vm/vm.hpp"

#include <cstdint>

namespace fetch {
namespace vm {

namespace {

template <typename To>
To Cast(Variant const &from)
{
  To to;
  switch (from.type_id)
  {
  case TypeIds::Bool:
  {
    to = static_cast<To>(from.primitive.ui8);
    break;
  }
  case TypeIds::Int8:
  {
    to = static_cast<To>(from.primitive.i8);
    break;
  }
  case TypeIds::UInt8:
  {
    to = static_cast<To>(from.primitive.ui8);
    break;
  }
  case TypeIds::Int16:
  {
    to = static_cast<To>(from.primitive.i16);
    break;
  }
  case TypeIds::UInt16:
  {
    to = static_cast<To>(from.primitive.ui16);
    break;
  }
  case TypeIds::Int32:
  {
    to = static_cast<To>(from.primitive.i32);
    break;
  }
  case TypeIds::UInt32:
  {
    to = static_cast<To>(from.primitive.ui32);
    break;
  }
  case TypeIds::Int64:
  {
    to = static_cast<To>(from.primitive.i64);
    break;
  }
  case TypeIds::UInt64:
  {
    to = static_cast<To>(from.primitive.ui64);
    break;
  }
  case TypeIds::Float32:
  {
    to = static_cast<To>(from.primitive.f32);
    break;
  }
  case TypeIds::Float64:
  {
    to = static_cast<To>(from.primitive.f64);
    break;
  }
  case TypeIds::Fixed32:
  {
    to = static_cast<To>(fixed_point::fp32_t::FromBase(from.primitive.i32));
    break;
  }
  case TypeIds::Fixed64:
  {
    to = static_cast<To>(fixed_point::fp64_t::FromBase(from.primitive.i64));
    break;
  }
  default:
  {
    to = 0;
    // Not a primitive
    assert(false);
    break;
  }
  }  // switch
  return to;
}

int8_t toInt8(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<int8_t>(from);
}

uint8_t toUInt8(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<uint8_t>(from);
}

int16_t toInt16(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<int16_t>(from);
}

uint16_t toUInt16(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<uint16_t>(from);
}

int32_t toInt32(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<int32_t>(from);
}

uint32_t toUInt32(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<uint32_t>(from);
}

int64_t toInt64(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<int64_t>(from);
}

uint64_t toUInt64(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<uint64_t>(from);
}

float toFloat32(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<float>(from);
}

double toFloat64(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<double>(from);
}

fixed_point::fp32_t toFixed32(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<fixed_point::fp32_t>(from);
}

fixed_point::fp64_t toFixed64(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<fixed_point::fp64_t>(from);
}

}  // namespace

Module::Module()
{
  CreateFreeFunction("toInt8", &toInt8, 1);
  CreateFreeFunction("toUInt8", &toUInt8, 1);
  CreateFreeFunction("toInt16", &toInt16, 1);
  CreateFreeFunction("toUInt16", &toUInt16, 1);
  CreateFreeFunction("toInt32", &toInt32, 1);
  CreateFreeFunction("toUInt32", &toUInt32, 1);
  CreateFreeFunction("toInt64", &toInt64, 1);
  CreateFreeFunction("toUInt64", &toUInt64, 1);
  CreateFreeFunction("toFloat32", &toFloat32, 1);
  CreateFreeFunction("toFloat64", &toFloat64, 1);
  CreateFreeFunction("toFixed32", &toFixed32, 1);
  CreateFreeFunction("toFixed64", &toFixed64, 1);

  auto const imatrix_ctor_estimator   = 1;
  auto const imatrix_getter_estimator = 1;
  auto const imatrix_setter_estimator = 1;

  GetClassInterface<IMatrix>()
      .CreateConstuctor<int32_t, int32_t>(std::move(imatrix_ctor_estimator))
      .EnableIndexOperator<decltype(imatrix_getter_estimator), decltype(imatrix_setter_estimator),
                           AnyInteger, AnyInteger, TemplateParameter1>(
          std::move(imatrix_getter_estimator), std::move(imatrix_setter_estimator))
      .CreateInstantiationType<Matrix<double>>()
      .CreateInstantiationType<Matrix<float>>();

  auto const iarray_ctor_estimator   = 1;
  auto const iarray_getter_estimator = 1;
  auto const iarray_setter_estimator = 1;
  GetClassInterface<IArray>()
      .CreateConstuctor<int32_t>(std::move(iarray_ctor_estimator))
      .CreateSerializeDefaultConstuctor<int32_t>(static_cast<int32_t>(0))
      .CreateMemberFunction("append", &IArray::Append, 1)
      .CreateMemberFunction("count", &IArray::Count, 1)
      .CreateMemberFunction("erase", &IArray::Erase, 1)
      .CreateMemberFunction("extend", &IArray::Extend, 1)
      .CreateMemberFunction("popBack", &IArray::PopBackOne, 1)
      .CreateMemberFunction("popBack", &IArray::PopBackMany, 1)
      .CreateMemberFunction("popFront", &IArray::PopFrontOne, 1)
      .CreateMemberFunction("popFront", &IArray::PopFrontMany, 1)
      .CreateMemberFunction("reverse", &IArray::Reverse, 1)
      .EnableIndexOperator<decltype(iarray_getter_estimator), decltype(iarray_setter_estimator),
                           AnyInteger, TemplateParameter1>(std::move(iarray_getter_estimator),
                                                           std::move(iarray_setter_estimator))
      .CreateInstantiationType<Array<bool>>()
      .CreateInstantiationType<Array<int8_t>>()
      .CreateInstantiationType<Array<uint8_t>>()
      .CreateInstantiationType<Array<int16_t>>()
      .CreateInstantiationType<Array<uint16_t>>()
      .CreateInstantiationType<Array<int32_t>>()
      .CreateInstantiationType<Array<uint32_t>>()
      .CreateInstantiationType<Array<int64_t>>()
      .CreateInstantiationType<Array<uint64_t>>()
      .CreateInstantiationType<Array<float>>()
      .CreateInstantiationType<Array<double>>()
      .CreateInstantiationType<Array<fixed_point::fp32_t>>()
      .CreateInstantiationType<Array<fixed_point::fp64_t>>()
      .CreateInstantiationType<Array<Ptr<String>>>()
      .CreateInstantiationType<Array<Ptr<Address>>>();

  GetClassInterface<String>()
      .CreateSerializeDefaultConstuctor<>()
      .CreateMemberFunction("find", &String::Find, 1)
      .CreateMemberFunction("length", &String::Length, 1)
      .CreateMemberFunction("reverse", &String::Reverse, 1)
      .CreateMemberFunction("split", &String::Split, 1)
      .CreateMemberFunction("substr", &String::Substring, 1)
      .CreateMemberFunction("trim", &String::Trim, 1);

  auto const imap_ctor_estimator   = 1;
  auto const imap_getter_estimator = 1;
  auto const imap_setter_estimator = 1;
  GetClassInterface<IMap>()
      .CreateConstuctor<>(std::move(imap_ctor_estimator))
      .CreateMemberFunction("count", &IMap::Count, 1)
      .EnableIndexOperator<decltype(imap_getter_estimator), decltype(imap_setter_estimator),
                           TemplateParameter1, TemplateParameter2>(
          std::move(imap_getter_estimator), std::move(imap_setter_estimator));

  auto const address_ctor_estimator = 1;
  GetClassInterface<Address>()
      .CreateSerializeDefaultConstuctor<>()
      .CreateConstuctor<Ptr<String>>(std::move(address_ctor_estimator))
      .CreateMemberFunction("signedTx", &Address::HasSignedTx, 1);

  auto const istate_ctor_estimator1 = 1;
  auto const istate_ctor_estimator2 = 1;
  GetClassInterface<IState>()
      .CreateConstuctor<Ptr<String>>(std::move(istate_ctor_estimator1))
      .CreateConstuctor<Ptr<Address>>(std::move(istate_ctor_estimator2))
      .CreateMemberFunction("get", static_cast<TemplateParameter1 (IState::*)()>(&IState::Get), 1)
      .CreateMemberFunction(
          "get",
          static_cast<TemplateParameter1 (IState::*)(TemplateParameter1 const &)>(&IState::Get), 1)
      .CreateMemberFunction("set", &IState::Set, 1)
      .CreateMemberFunction("existed", &IState::Existed, 1);

  auto const ishardedstate_ctor_estimator1 = 1;
  auto const ishardedstate_ctor_estimator2 = 1;
  GetClassInterface<IShardedState>()
      .CreateConstuctor<Ptr<String>>(std::move(ishardedstate_ctor_estimator1))
      .CreateConstuctor<Ptr<Address>>(std::move(ishardedstate_ctor_estimator2))
      // TODO (issue 1172): This will be enabled once the issue is resolved
      //.EnableIndexOperator<Ptr<String>, TemplateParameter1>()
      //.EnableIndexOperator<Ptr<Address>, TemplateParameter1>();
      .CreateMemberFunction("get",
                            static_cast<TemplateParameter1 (IShardedState::*)(Ptr<String> const &)>(
                                &IShardedState::Get),
                            1)
      .CreateMemberFunction(
          "get",
          static_cast<TemplateParameter1 (IShardedState::*)(Ptr<Address> const &)>(
              &IShardedState::Get),
          1)
      .CreateMemberFunction(
          "get",
          static_cast<TemplateParameter1 (IShardedState::*)(
              Ptr<String> const &, TemplateParameter1 const &)>(&IShardedState::Get),
          1)
      .CreateMemberFunction(
          "get",
          static_cast<TemplateParameter1 (IShardedState::*)(
              Ptr<Address> const &, TemplateParameter1 const &)>(&IShardedState::Get),
          1)
      .CreateMemberFunction(
          "set",
          static_cast<void (IShardedState::*)(Ptr<String> const &, TemplateParameter1 const &)>(
              &IShardedState::Set),
          1)
      .CreateMemberFunction(
          "set",
          static_cast<void (IShardedState::*)(Ptr<Address> const &, TemplateParameter1 const &)>(
              &IShardedState::Set),
          1);
}

}  // namespace vm
}  // namespace fetch
