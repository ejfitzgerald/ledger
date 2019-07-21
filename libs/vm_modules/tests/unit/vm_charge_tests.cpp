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
using fetch::vm::ConstantEstimator;

VM::ChargeAmount const low_charge_limit  = 10;
VM::ChargeAmount const high_charge_limit = 1000;

auto const affordable_estimator = [](VM *, uint8_t x, uint16_t y) -> VM::ChargeAmount {
  return static_cast<VM::ChargeAmount>(low_charge_limit + x * y);
};
auto const expensive_estimator = [](VM *, uint8_t x, uint16_t y) -> VM::ChargeAmount {
  return static_cast<VM::ChargeAmount>(high_charge_limit + x * y);
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
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
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
      .CreateConstuctor<decltype(expensive_estimator), uint8_t, uint16_t>(
          std::move(expensive_estimator));

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

TEST_F(VmChargeTests, member_function_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  auto const ctor_estimator = ConstantEstimator<0>::Get();

  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstuctor<decltype(ctor_estimator)>(std::move(ctor_estimator))
      .CreateMemberFunction("tooExpensive", &CustomType::TooExpensive,
                            std::move(expensive_estimator));

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
  auto const ctor_estimator = ConstantEstimator<0>::Get();

  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstuctor<decltype(ctor_estimator)>(std::move(ctor_estimator))
      .CreateMemberFunction("affordable", &CustomType::Affordable, std::move(affordable_estimator));

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
                                  std::move(expensive_estimator));

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
                                  std::move(affordable_estimator));

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
  auto const ctor_estimator   = ConstantEstimator<0>::Get();
  auto const getter_estimator = ConstantEstimator<1>::Get(10000);
  auto const setter_estimator = ConstantEstimator<2>::Get(10000);

  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstuctor<decltype(ctor_estimator)>(std::move(ctor_estimator))
      .EnableIndexOperator<decltype(getter_estimator), decltype(setter_estimator), int32_t,
                           int32_t>(std::move(getter_estimator), std::move(setter_estimator));

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
  auto const ctor_estimator   = ConstantEstimator<0>::Get();
  auto const getter_estimator = ConstantEstimator<1>::Get();
  auto const setter_estimator = ConstantEstimator<2>::Get();

  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstuctor<decltype(ctor_estimator)>(std::move(ctor_estimator))
      .EnableIndexOperator<decltype(getter_estimator), decltype(setter_estimator), int32_t,
                           int32_t>(std::move(getter_estimator), std::move(setter_estimator));

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
