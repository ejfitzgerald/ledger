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

#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include <utility>

namespace fetch {
namespace vm_modules {
namespace ml {

VMTrainingPair::VMTrainingPair(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                               fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ta,
                               fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> tb)
  : fetch::vm::Object(vm, type_id)
{
  this->first  = ta;
  this->second = tb;
}

fetch::vm::Ptr<VMTrainingPair> VMTrainingPair::Constructor(
    fetch::vm::VM *vm, fetch::vm::TypeId type_id,
    fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ta,
    fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> tb)
{
  return new VMTrainingPair(vm, type_id, ta, tb);
}

void VMTrainingPair::Bind(vm::Module &module)
{
  auto const training_pair_ctor_estimator = 1;

  module.CreateClassType<fetch::vm_modules::ml::VMTrainingPair>("TrainingPair")
      .CreateConstuctor<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>,
                        fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>(
          std::move(training_pair_ctor_estimator))
      .CreateMemberFunction("data", &fetch::vm_modules::ml::VMTrainingPair::data, 1)
      .CreateMemberFunction("label", &fetch::vm_modules::ml::VMTrainingPair::label, 1);
}

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> VMTrainingPair::data()
{
  return this->second;
}

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> VMTrainingPair::label()
{
  return this->first;
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
