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

#include "ml/graph.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/optimisation/adam_optimiser.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace vm_modules {
namespace ml {

VMAdamOptimiser::VMAdamOptimiser(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                 GraphType const &               graph,
                                 std::vector<std::string> const &input_node_names,
                                 std::string const &             label_node_name,
                                 std::string const &             output_node_name)
  : fetch::vm::Object(vm, type_id)
  , optimiser_(std::make_shared<GraphType>(graph), input_node_names, label_node_name,
               output_node_name)
{}

fetch::vm::Ptr<VMAdamOptimiser> VMAdamOptimiser::Constructor(
    fetch::vm::VM *vm, fetch::vm::TypeId type_id,
    fetch::vm::Ptr<fetch::vm_modules::ml::VMGraph> const &graph,
    fetch::vm::Ptr<fetch::vm::String> const &             input_node_names,
    fetch::vm::Ptr<fetch::vm::String> const &             label_node_name,
    fetch::vm::Ptr<fetch::vm::String> const &             output_node_names)
{
  return new VMAdamOptimiser(vm, type_id, graph->graph_, {input_node_names->str},
                             label_node_name->str, output_node_names->str);
}

void VMAdamOptimiser::Bind(vm::Module &module)
{
  auto const adam_optimiser_ctor_estimator = 1;

  module.CreateClassType<fetch::vm_modules::ml::VMAdamOptimiser>("AdamOptimiser")
      .CreateConstuctor<fetch::vm::Ptr<fetch::vm_modules::ml::VMGraph>,
                        fetch::vm::Ptr<fetch::vm::String>, fetch::vm::Ptr<fetch::vm::String>,
                        fetch::vm::Ptr<fetch::vm::String>>(std::move(adam_optimiser_ctor_estimator))
      .CreateMemberFunction("run", &fetch::vm_modules::ml::VMAdamOptimiser::RunData, 1)
      .CreateMemberFunction("run", &fetch::vm_modules::ml::VMAdamOptimiser::RunLoader, 1);
}

VMAdamOptimiser::DataType VMAdamOptimiser::RunData(
    fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &data,
    fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &labels, uint64_t batch_size)
{
  return optimiser_.Run({(data->GetTensor())}, labels->GetTensor(), batch_size);
}

VMAdamOptimiser::DataType VMAdamOptimiser::RunLoader(
    fetch::vm::Ptr<fetch::vm_modules::ml::VMDataLoader> const &loader, uint64_t batch_size,
    uint64_t subset_size)
{
  return optimiser_.Run(*(loader->loader_), batch_size, subset_size);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
