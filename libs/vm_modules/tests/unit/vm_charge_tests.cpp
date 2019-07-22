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

using fetch::vm::TypeId;
using fetch::vm::VM;

ChargeAmount const low_charge_limit  = 10;
ChargeAmount const high_charge_limit = 1000;

ChargeAmount AffordableEstimator(VM *, uint8_t x, uint16_t y)
{
  return static_cast<ChargeAmount>(low_charge_limit + x * y);
}

ChargeAmount ExpensiveEstimator(VM *, uint8_t x, uint16_t y)
{
  return static_cast<ChargeAmount>(high_charge_limit + x * y);
}

bool Handler(VM *, uint8_t, uint16_t)
{
  return true;
}

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
  toolkit.module().CreateFreeFunction("tooExpensive", &Handler, fetch::vm::ChargeEstimator<uint8_t, uint16_t>{&ExpensiveEstimator});

  static char const *TEXT = R"(
    function main()
      var check = tooExpensive(3u8, 4u16);
      assert(check == true);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, functor_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module().CreateFreeFunction("affordable", &Handler, fetch::vm::ChargeEstimator<uint8_t, uint16_t>{&AffordableEstimator});

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
  CustomType(VM *vm, TypeId type_id, uint8_t, uint16_t)
    : fetch::vm::Object{vm, type_id}
  {}
  ~CustomType() override = default;

  static fetch::vm::Ptr<CustomType> Constructor(VM *vm, TypeId type_id)
  {
    return new CustomType{vm, type_id, 0, 0};
  }

  static fetch::vm::Ptr<CustomType> Constructor(VM *vm, TypeId type_id, uint8_t x, uint16_t y)
  {
    return new CustomType{vm, type_id, x, y};
  }

  static void AffordableStatic(VM *, TypeId, uint8_t, uint16_t)
  {}

  static void TooExpensiveStatic(VM *, TypeId, uint8_t, uint16_t)
  {}

  void Affordable(uint8_t, uint16_t)
  {}

  void TooExpensive(uint8_t, uint16_t)
  {}

  int32_t GetIndexedValue(int32_t)
  {
    return 0;
  }

  void SetIndexedValue(int32_t, int32_t)
  {}
};

TEST_F(VmChargeTests, ctor_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module()
      .CreateClassType<CustomType>("TooExpensive")
      .CreateConstuctor<uint8_t, uint16_t>(1000);

  static char const *TEXT = R"(
    function main()
      TooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, ctor_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module()
      .CreateClassType<CustomType>("Affordable")
      .CreateConstuctor<uint8_t, uint16_t>(10);

  static char const *TEXT = R"(
    function main()
      Affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, member_function_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstuctor<>(1)
      .CreateMemberFunction("tooExpensive", &CustomType::TooExpensive, fetch::vm::ChargeEstimator<uint8_t, uint16_t>{&ExpensiveEstimator});

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj.tooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       member_function_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstuctor<>(1)
      .CreateMemberFunction("affordable", &CustomType::Affordable, fetch::vm::ChargeEstimator<uint8_t, uint16_t>{&AffordableEstimator});

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj.affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests,
       static_member_function_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateStaticMemberFunction("tooExpensive", &CustomType::TooExpensiveStatic,
                                  fetch::vm::ChargeEstimator<uint8_t, uint16_t>{&ExpensiveEstimator});

  static char const *TEXT = R"(
    function main()
      CustomType.tooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       static_member_function_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateStaticMemberFunction("affordable", &CustomType::AffordableStatic,
                                  fetch::vm::ChargeEstimator<uint8_t, uint16_t>{&AffordableEstimator});

  static char const *TEXT = R"(
    function main()
      CustomType.affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, index_operator_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstuctor<>(1)
      .EnableIndexOperator<int32_t, int32_t>(1000, 1000);

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj[3];
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, index_operator_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstuctor<>(1)
      .EnableIndexOperator<int32_t, int32_t>(10, 10);

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj[3];
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

}  // namespace
