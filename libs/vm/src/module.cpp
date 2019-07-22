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
  static constexpr uint64_t DEFAULT_CHARGE = 1;

  CreateFreeFunction("toInt8", &toInt8, DEFAULT_CHARGE);
  CreateFreeFunction("toUInt8", &toUInt8, DEFAULT_CHARGE);
  CreateFreeFunction("toInt16", &toInt16, DEFAULT_CHARGE);
  CreateFreeFunction("toUInt16", &toUInt16, DEFAULT_CHARGE);
  CreateFreeFunction("toInt32", &toInt32, DEFAULT_CHARGE);
  CreateFreeFunction("toUInt32", &toUInt32, DEFAULT_CHARGE);
  CreateFreeFunction("toInt64", &toInt64, DEFAULT_CHARGE);
  CreateFreeFunction("toUInt64", &toUInt64, DEFAULT_CHARGE);
  CreateFreeFunction("toFloat32", &toFloat32, DEFAULT_CHARGE);
  CreateFreeFunction("toFloat64", &toFloat64, DEFAULT_CHARGE);
  CreateFreeFunction("toFixed32", &toFixed32, DEFAULT_CHARGE);
  CreateFreeFunction("toFixed64", &toFixed64, DEFAULT_CHARGE);

  GetClassInterface<IMatrix>()
      .CreateConstuctor<int32_t, int32_t>(DEFAULT_CHARGE)
      .EnableIndexOperator<AnyInteger, AnyInteger, TemplateParameter1>(DEFAULT_CHARGE,DEFAULT_CHARGE)
      .CreateInstantiationType<Matrix<double>>()
      .CreateInstantiationType<Matrix<float>>();

  GetClassInterface<IArray>()
      .CreateConstuctor<int32_t>(DEFAULT_CHARGE)
      .CreateSerializeDefaultConstuctor<int32_t>(static_cast<int32_t>(0))
      .CreateMemberFunction("append", &IArray::Append, DEFAULT_CHARGE)
      .CreateMemberFunction("count", &IArray::Count, DEFAULT_CHARGE)
      .CreateMemberFunction("erase", &IArray::Erase, DEFAULT_CHARGE)
      .CreateMemberFunction("extend", &IArray::Extend, DEFAULT_CHARGE)
      .CreateMemberFunction("popBack", &IArray::PopBackOne, DEFAULT_CHARGE)
      .CreateMemberFunction("popBack", &IArray::PopBackMany, DEFAULT_CHARGE)
      .CreateMemberFunction("popFront", &IArray::PopFrontOne, DEFAULT_CHARGE)
      .CreateMemberFunction("popFront", &IArray::PopFrontMany, DEFAULT_CHARGE)
      .CreateMemberFunction("reverse", &IArray::Reverse, DEFAULT_CHARGE)
      .EnableIndexOperator<AnyInteger, TemplateParameter1>(DEFAULT_CHARGE, DEFAULT_CHARGE)
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
      .CreateMemberFunction("find", &String::Find, DEFAULT_CHARGE)
      .CreateMemberFunction("length", &String::Length, DEFAULT_CHARGE)
      .CreateMemberFunction("reverse", &String::Reverse, DEFAULT_CHARGE)
      .CreateMemberFunction("split", &String::Split, DEFAULT_CHARGE)
      .CreateMemberFunction("substr", &String::Substring, DEFAULT_CHARGE)
      .CreateMemberFunction("trim", &String::Trim, DEFAULT_CHARGE);

  GetClassInterface<IMap>()
      .CreateConstuctor<>(DEFAULT_CHARGE)
      .CreateMemberFunction("count", &IMap::Count, DEFAULT_CHARGE)
      .EnableIndexOperator<TemplateParameter1, TemplateParameter2>(DEFAULT_CHARGE,DEFAULT_CHARGE);

  GetClassInterface<Address>()
      .CreateSerializeDefaultConstuctor<>()
      .CreateConstuctor<Ptr<String>>(DEFAULT_CHARGE)
      .CreateMemberFunction("signedTx", &Address::HasSignedTx, DEFAULT_CHARGE);

  GetClassInterface<IState>()
      .CreateConstuctor<Ptr<String>>(DEFAULT_CHARGE)
      .CreateConstuctor<Ptr<Address>>(DEFAULT_CHARGE)
      .CreateMemberFunction("get", static_cast<TemplateParameter1 (IState::*)()>(&IState::Get), DEFAULT_CHARGE)
      .CreateMemberFunction(
          "get",
          static_cast<TemplateParameter1 (IState::*)(TemplateParameter1 const &)>(&IState::Get), DEFAULT_CHARGE)
      .CreateMemberFunction("set", &IState::Set, DEFAULT_CHARGE)
      .CreateMemberFunction("existed", &IState::Existed, DEFAULT_CHARGE);

  GetClassInterface<IShardedState>()
      .CreateConstuctor<Ptr<String>>(DEFAULT_CHARGE)
      .CreateConstuctor<Ptr<Address>>(DEFAULT_CHARGE)
      // TODO (issue 1172): This will be enabled once the issue is resolved
      //.EnableIndexOperator<Ptr<String>, TemplateParameter1>()
      //.EnableIndexOperator<Ptr<Address>, TemplateParameter1>();
      .CreateMemberFunction("get",
                            static_cast<TemplateParameter1 (IShardedState::*)(Ptr<String> const &)>(
                                &IShardedState::Get),
                            DEFAULT_CHARGE)
      .CreateMemberFunction(
          "get",
          static_cast<TemplateParameter1 (IShardedState::*)(Ptr<Address> const &)>(
              &IShardedState::Get),
          DEFAULT_CHARGE)
      .CreateMemberFunction(
          "get",
          static_cast<TemplateParameter1 (IShardedState::*)(
              Ptr<String> const &, TemplateParameter1 const &)>(&IShardedState::Get),
          DEFAULT_CHARGE)
      .CreateMemberFunction(
          "get",
          static_cast<TemplateParameter1 (IShardedState::*)(
              Ptr<Address> const &, TemplateParameter1 const &)>(&IShardedState::Get),
          DEFAULT_CHARGE)
      .CreateMemberFunction(
          "set",
          static_cast<void (IShardedState::*)(Ptr<String> const &, TemplateParameter1 const &)>(
              &IShardedState::Set),
          DEFAULT_CHARGE)
      .CreateMemberFunction(
          "set",
          static_cast<void (IShardedState::*)(Ptr<Address> const &, TemplateParameter1 const &)>(
              &IShardedState::Set),
          DEFAULT_CHARGE);
}

}  // namespace vm
}  // namespace fetch
