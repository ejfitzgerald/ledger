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

#include <memory>

namespace {

using fetch::vm::VM;

class VmChargeTests : public ::testing::Test
{
public:
  //  std::stringstream stdout;
  VmTestToolkit toolkit;
  //  VmTestToolkit     toolkit{&stdout};
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
  toolkit.vm().SetChargeLimit(1000);
  ASSERT_EQ(1000, toolkit.vm().GetChargeLimit());
  ASSERT_TRUE(toolkit.Run());
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
  toolkit.vm().SetChargeLimit(10);
  ASSERT_EQ(10, toolkit.vm().GetChargeLimit());
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(VmChargeTests, bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  auto handler   = [](VM *, int32_t, int32_t) -> bool { return true; };
  auto estimator = [](VM *, int32_t x, int32_t y) -> VM::ChargeAmount {
    auto const base = x * y;
    return static_cast<VM::ChargeAmount>(1000 + base * base);
  };

  toolkit.module().CreateFreeFunction("tooExpensive", std::move(handler), std::move(estimator));

  static char const *TEXT = R"(
    function main()
      var check = tooExpensive(3, 4);
      assert(check == true);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  toolkit.vm().SetChargeLimit(1000);
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(VmChargeTests, bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  auto handler   = [](VM *, int32_t, int32_t) -> bool { return true; };
  auto estimator = [](VM *, int32_t x, int32_t y) -> VM::ChargeAmount {
    auto const base = x * y;
    return static_cast<VM::ChargeAmount>(1 + base * base);
  };

  toolkit.module().CreateFreeFunction("affordable", std::move(handler), std::move(estimator));

  static char const *TEXT = R"(
    function main()
      var check = affordable(3, 4);
      assert(check == true);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  toolkit.vm().SetChargeLimit(1000);
  ASSERT_TRUE(toolkit.Run());
}

}  // namespace
