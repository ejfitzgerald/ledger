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

#include "math/tensor.hpp"
#include "ml/graph.hpp"
#include "vm/object.hpp"
#include "vm_modules/ml/state_dict.hpp"

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {
namespace ml {

class VMGraph : public fetch::vm::Object
{
  using SizeType       = fetch::math::SizeType;
  using MathTensorType = fetch::math::Tensor<float>;
  using VMTensorType   = fetch::vm_modules::math::VMTensor;
  using GraphType      = fetch::ml::Graph<MathTensorType>;
  using VMPtrString    = fetch::vm::Ptr<fetch::vm::String>;

public:
  VMGraph(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMGraph> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static void Bind(fetch::vm::Module &module);

  void SetInput(VMPtrString const &name, fetch::vm::Ptr<VMTensorType> const &input);
  fetch::vm::Ptr<VMTensorType> Evaluate(VMPtrString const &name);
  void                         BackPropagateError(VMPtrString const &name);
  void                         Step(float lr);

  void AddPlaceholder(VMPtrString const &name);
  void AddFullyConnected(VMPtrString const &name, VMPtrString const &input_name, int in, int out);
  void AddConv1D(VMPtrString const &name, VMPtrString const &input_name, int out_channels,
                 int in_channels, int kernel_size, int stride_size);
  void AddRelu(VMPtrString const &name, VMPtrString const &input_name);
  void AddSoftmax(VMPtrString const &name, VMPtrString const &input_name);
  void AddCrossEntropyLoss(VMPtrString const &name, VMPtrString const &input_name,
                           VMPtrString const &label_name);
  void AddMeanSquareErrorLoss(VMPtrString const &name, VMPtrString const &input_name,
                              VMPtrString const &label_name);
  void AddDropout(VMPtrString const &name, VMPtrString const &input_name, float const &prob);

  void                        LoadStateDict(fetch::vm::Ptr<VMStateDict> const &sd);
  fetch::vm::Ptr<VMStateDict> StateDict();

  GraphType graph_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
