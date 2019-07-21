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

#include "vm_test_toolkit.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace {

using fetch::vm::VM;

VM::ChargeAmount const low_charge_limit  = 10;
VM::ChargeAmount const high_charge_limit = 1000;

auto const affordable_estimator = [](VM *, uint8_t x, uint16_t y) -> VM::ChargeAmount {
  auto const base = static_cast<VM::ChargeAmount>(x * y);
  return static_cast<VM::ChargeAmount>(low_charge_limit + base * base);
};
auto const expensive_estimator = [](VM *, uint8_t x, uint16_t y) -> VM::ChargeAmount {
  auto const base = static_cast<VM::ChargeAmount>(x * y);
  return static_cast<VM::ChargeAmount>(high_charge_limit + base * base);
};

auto const handler = [](VM *, uint8_t, uint16_t) -> bool { return true; };

class VmChargeTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(VmChargeTests, execution_succeeds_when_charge_limit_obeyed)
{
  static char const *TEXT = R"(
    function main()
      var u = 42u8;
      print(u);
      print(', ');

      var pos_i = 42i8;
      print(pos_i);
      print(', ');

      var neg_i = -42i8;
      print(neg_i);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, execution_fails_when_charge_limit_exceeded)
{
  static char const *TEXT = R"(
    function main()
      var u = 42u8;
      print(u);
      print(', ');

      var pos_i = 42i8;
      print(pos_i);
      print(', ');

      var neg_i = -42i8;
      print(neg_i);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, functor_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module().CreateFreeFunction("tooExpensive", std::move(handler), expensive_estimator);

  static char const *TEXT = R"(
    function main()
      var check = tooExpensive(3u8, 4u16);
      assert(check == true);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, functor_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module().CreateFreeFunction("affordable", std::move(handler), affordable_estimator);

  static char const *TEXT = R"(
    function main()
      var check = affordable(3u8, 4u16);
      assert(check == true);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

class CustomType : public fetch::vm::Object
{
public:
  CustomType(fetch::vm::VM *vm, fetch::vm::TypeId type_id, uint8_t, uint16_t)
    : fetch::vm::Object{vm, type_id}
  {}

  static fetch::vm::Ptr<CustomType> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                uint8_t x, uint16_t y)
  {
    return new CustomType{vm, type_id, x, y};
  }
};

TEST_F(VmChargeTests, ctor_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module()
      .CreateClassType<CustomType>("TooExpensive")
      .CreateConstuctor<decltype(expensive_estimator), uint8_t, uint16_t>(
          std::move(expensive_estimator));

  static char const *TEXT = R"(
    function main()
      TooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, ctor_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module()
      .CreateClassType<CustomType>("Affordable")
      .CreateConstuctor<decltype(affordable_estimator), uint8_t, uint16_t>(
          std::move(affordable_estimator));

  static char const *TEXT = R"(
    function main()
      Affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

}  // namespace
