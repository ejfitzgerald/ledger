#pragma once
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

#include "crypto/sha256.hpp"

#include <cstdint>

namespace fetch {
namespace vm_modules {

class ByteArrayWrapper;
namespace math {
class UInt256Wrapper;
}

class SHA256Wrapper : public fetch::vm::Object
{
public:
  using ElementType = uint8_t;

  template <typename T>
  using Ptr    = fetch::vm::Ptr<T>;
  using String = fetch::vm::String;

  SHA256Wrapper()           = delete;
  ~SHA256Wrapper() override = default;
  SHA256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<SHA256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static void Bind(vm::Module &module);

  void                      UpdateUInt256(vm::Ptr<vm_modules::math::UInt256Wrapper> const &uint);
  void                      UpdateString(vm::Ptr<vm::String> const &str);
  void                      UpdateBuffer(Ptr<ByteArrayWrapper> const &buffer);
  void                      Reset();
  Ptr<math::UInt256Wrapper> Final();

private:
  crypto::SHA256 hasher_;
};

}  // namespace vm_modules
}  // namespace fetch
