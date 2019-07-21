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

#include "vm/object.hpp"

#include <cstdint>

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {

class ByteArrayWrapper : public fetch::vm::Object
{
public:
  ByteArrayWrapper()           = delete;
  ~ByteArrayWrapper() override = default;

  ByteArrayWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                   byte_array::ByteArray const &bytearray);

  static fetch::vm::Ptr<ByteArrayWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                      int32_t n);
  static fetch::vm::Ptr<ByteArrayWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                      byte_array::ByteArray bytearray);

  static void Bind(vm::Module &module);

  fetch::vm::Ptr<ByteArrayWrapper> Copy() const;
  byte_array::ByteArray            byte_array() const;
  bool                             SerializeTo(vm::ByteArrayBuffer &buffer) override;
  bool                             DeserializeFrom(vm::ByteArrayBuffer &buffer) override;

private:
  byte_array::ByteArray byte_array_;
};

}  // namespace vm_modules
}  // namespace fetch
