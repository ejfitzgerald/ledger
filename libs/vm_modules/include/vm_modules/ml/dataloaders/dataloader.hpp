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

#include "ml/dataloaders/dataloader.hpp"
#include "vm/object.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include <memory>

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {
namespace ml {

class VMDataLoader : public fetch::vm::Object
{
public:
  VMDataLoader(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMDataLoader> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static void Bind(fetch::vm::Module &module);

  void                           AddData(fetch::vm::Ptr<fetch::vm::String> const &mode,
                                         fetch::vm::Ptr<fetch::vm::String> const &xfilename,
                                         fetch::vm::Ptr<fetch::vm::String> const &yfilename);
  fetch::vm::Ptr<VMTrainingPair> GetNext();
  bool                           IsDone();

  std::shared_ptr<
      fetch::ml::dataloaders::DataLoader<fetch::math::Tensor<float>, fetch::math::Tensor<float>>>
      loader_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
