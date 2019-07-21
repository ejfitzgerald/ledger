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

#include "ml/state_dict.hpp"
#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/state_dict.hpp"

#include <utility>

using fetch::vm::ConstantEstimator;

namespace fetch {
namespace vm_modules {
namespace ml {

VMStateDict::VMStateDict(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  : fetch::vm::Object(vm, type_id)
  , state_dict_()
{}

VMStateDict::VMStateDict(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                         fetch::ml::StateDict<MathTensorType> sd)
  : fetch::vm::Object(vm, type_id)
{
  state_dict_ = std::move(sd);
}

fetch::vm::Ptr<VMStateDict> VMStateDict::Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
{
  return new VMStateDict(vm, type_id);
}

void VMStateDict::Bind(fetch::vm::Module &module)
{
  auto const statedict_ctor_estimator = ConstantEstimator<0>::Get();

  module.CreateClassType<VMStateDict>("StateDict")
      .CreateConstuctor<decltype(statedict_ctor_estimator)>(std::move(statedict_ctor_estimator))
      .CreateMemberFunction("setWeights", &VMStateDict::SetWeights, ConstantEstimator<2>::Get());
}

void VMStateDict::SetWeights(fetch::vm::Ptr<fetch::vm::String> const &                nodename,
                             fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &weights)
{
  auto weights_tensor = state_dict_.dict_[nodename->str].weights_;
  *weights_tensor     = weights->GetTensor();
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch