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

#include "math/tensor.hpp"
#include "ml/dataloaders/commodity_dataloader.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

using fetch::vm::ConstantEstimator;

namespace fetch {
namespace vm_modules {
namespace ml {

VMDataLoader::VMDataLoader(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  : fetch::vm::Object(vm, type_id)
{}

fetch::vm::Ptr<VMDataLoader> VMDataLoader::Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
{
  return new VMDataLoader(vm, type_id);
}

void VMDataLoader::Bind(fetch::vm::Module &module)
{
  auto const dataloader_ctor_estimator = ConstantEstimator<0>::Get();

  module.CreateClassType<VMDataLoader>("DataLoader")
      .CreateConstuctor<decltype(dataloader_ctor_estimator)>(std::move(dataloader_ctor_estimator))
      .CreateMemberFunction("addData", &VMDataLoader::AddData, ConstantEstimator<3>::Get())
      .CreateMemberFunction("getNext", &VMDataLoader::GetNext, ConstantEstimator<0>::Get())
      .CreateMemberFunction("isDone", &VMDataLoader::IsDone, ConstantEstimator<0>::Get());
}

void VMDataLoader::AddData(fetch::vm::Ptr<fetch::vm::String> const &mode,
                           fetch::vm::Ptr<fetch::vm::String> const &xfilename,
                           fetch::vm::Ptr<fetch::vm::String> const &yfilename)
{
  if (mode->str == "commodity")
  {
    fetch::ml::dataloaders::CommodityDataLoader<fetch::math::Tensor<float>,
                                                fetch::math::Tensor<float>>
        ld;
    ld.AddData(xfilename->str, yfilename->str);

    loader_ = std::make_shared<fetch::ml::dataloaders::CommodityDataLoader<
        fetch::math::Tensor<float>, fetch::math::Tensor<float>>>(ld);
  }
  else if (mode->str == "mnist")
  {

    loader_ = std::make_shared<fetch::ml::dataloaders::MNISTLoader<fetch::math::Tensor<float>,
                                                                   fetch::math::Tensor<float>>>(
        xfilename->str, yfilename->str);
  }
  else
  {
    throw std::runtime_error("Unrecognised loader type");
  }
}

fetch::vm::Ptr<VMTrainingPair> VMDataLoader::GetNext()
{
  std::pair<fetch::math::Tensor<float>, std::vector<fetch::math::Tensor<float>>> next =
      loader_->GetNext();

  auto first      = this->vm_->CreateNewObject<math::VMTensor>(next.first);
  auto second     = this->vm_->CreateNewObject<math::VMTensor>(next.second.at(0));
  auto dataHolder = this->vm_->CreateNewObject<VMTrainingPair>(first, second);

  return dataHolder;
}

bool VMDataLoader::IsDone()
{
  return loader_->IsDone();
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
