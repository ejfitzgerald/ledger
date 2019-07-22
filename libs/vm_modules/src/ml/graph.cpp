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
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "vm/module.hpp"
#include "vm/module/estimate_charge.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/state_dict.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {

VMGraph::VMGraph(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  : fetch::vm::Object(vm, type_id)
  , graph_()
{}

fetch::vm::Ptr<VMGraph> VMGraph::Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
{
  return new VMGraph(vm, type_id);
}

void VMGraph::Bind(fetch::vm::Module &module)
{
  auto const graph_ctor_estimator = 1;

  module.CreateClassType<VMGraph>("Graph")
      .CreateConstuctor<>(std::move(graph_ctor_estimator))
      .CreateMemberFunction("setInput", &VMGraph::SetInput, 1)
      .CreateMemberFunction("evaluate", &VMGraph::Evaluate, 1)
      .CreateMemberFunction("backPropagate", &VMGraph::BackPropagateError, 1)
      .CreateMemberFunction("step", &VMGraph::Step, 1)
      .CreateMemberFunction("addPlaceholder", &VMGraph::AddPlaceholder, 1)
      .CreateMemberFunction("addFullyConnected", &VMGraph::AddFullyConnected, 1)
      .CreateMemberFunction("addConv1D", &VMGraph::AddConv1D, 1)
      .CreateMemberFunction("addRelu", &VMGraph::AddRelu, 1)
      .CreateMemberFunction("addSoftmax", &VMGraph::AddSoftmax, 1)
      .CreateMemberFunction("addDropout", &VMGraph::AddDropout, 1)
      .CreateMemberFunction("addCrossEntropyLoss", &VMGraph::AddCrossEntropyLoss, 1)
      .CreateMemberFunction("loadStateDict", &VMGraph::LoadStateDict, 1)
      .CreateMemberFunction("stateDict", &VMGraph::StateDict, 1);
}

void VMGraph::SetInput(VMPtrString const &name, fetch::vm::Ptr<VMTensorType> const &input)
{
  graph_.SetInput(name->str, (*input).GetTensor());
}

fetch::vm::Ptr<VMGraph::VMTensorType> VMGraph::Evaluate(VMPtrString const &name)
{
  MathTensorType               t   = graph_.Evaluate(name->str);
  fetch::vm::Ptr<VMTensorType> ret = this->vm_->CreateNewObject<math::VMTensor>(t.shape());
  (*ret).Copy(t);
  return ret;
}

void VMGraph::BackPropagateError(VMPtrString const &name)
{
  graph_.BackPropagateError(name->str);
}

void VMGraph::Step(float lr)
{
  graph_.Step(lr);
}

void VMGraph::AddPlaceholder(VMPtrString const &name)
{
  graph_.AddNode<fetch::ml::ops::PlaceHolder<MathTensorType>>(name->str, {});
}

void VMGraph::AddFullyConnected(VMPtrString const &name, VMPtrString const &input_name, int in,
                                int out)
{
  graph_.AddNode<fetch::ml::layers::FullyConnected<MathTensorType>>(
      name->str, {input_name->str}, std::size_t(in), std::size_t(out));
}

void VMGraph::AddConv1D(VMPtrString const &name, VMPtrString const &input_name, int out_channels,
                        int in_channels, int kernel_size, int stride_size)
{
  graph_.AddNode<fetch::ml::layers::Convolution1D<MathTensorType>>(
      name->str, {input_name->str}, static_cast<SizeType>(out_channels),
      static_cast<SizeType>(in_channels), static_cast<SizeType>(kernel_size),
      static_cast<SizeType>(stride_size));
}

void VMGraph::AddRelu(VMPtrString const &name, VMPtrString const &input_name)
{
  graph_.AddNode<fetch::ml::ops::Relu<MathTensorType>>(name->str, {input_name->str});
}

void VMGraph::AddSoftmax(VMPtrString const &name, VMPtrString const &input_name)
{
  graph_.AddNode<fetch::ml::ops::Softmax<fetch::math::Tensor<float>>>(name->str, {input_name->str});
}

void VMGraph::AddCrossEntropyLoss(VMPtrString const &name, VMPtrString const &input_name,
                                  VMPtrString const &label_name)
{
  graph_.AddNode<fetch::ml::ops::CrossEntropyLoss<fetch::math::Tensor<float>>>(
      name->str, {input_name->str, label_name->str});
}

void VMGraph::AddMeanSquareErrorLoss(VMPtrString const &name, VMPtrString const &input_name,
                                     VMPtrString const &label_name)
{
  graph_.AddNode<fetch::ml::ops::MeanSquareErrorLoss<fetch::math::Tensor<float>>>(
      name->str, {input_name->str, label_name->str});
}

void VMGraph::AddDropout(VMPtrString const &name, VMPtrString const &input_name, float const &prob)
{
  graph_.AddNode<fetch::ml::ops::Dropout<MathTensorType>>(name->str, {input_name->str}, prob);
}

void VMGraph::LoadStateDict(fetch::vm::Ptr<VMStateDict> const &sd)
{
  graph_.LoadStateDict(sd->state_dict_);
}

fetch::vm::Ptr<VMStateDict> VMGraph::StateDict()
{
  fetch::vm::Ptr<VMStateDict> ret = this->vm_->CreateNewObject<VMStateDict>(graph_.StateDict());
  return ret;
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
