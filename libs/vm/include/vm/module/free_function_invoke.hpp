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

#include "estimate_charge.hpp"

#include <utility>

namespace fetch {
namespace vm {

template <typename ReturnType, typename FreeFunction, typename Estimator, typename... Ts>
struct FreeFunctionInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction f, Estimator &&e,
                     Ts const &... parameters)
  {
    if (EstimateCharge(vm, std::forward<Estimator>(e), parameters...))
    {
      ReturnType result((*f)(vm, parameters...));
      StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
      vm->sp_ -= sp_offset;
    }
  }
};

template <typename FreeFunction, typename Estimator, typename... Ts>
struct FreeFunctionInvokerHelper<void, FreeFunction, Estimator, Ts...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId /* return_type_id */, FreeFunction f,
                     Estimator &&e, Ts const &... parameters)
  {
    if (EstimateCharge(vm, std::forward<Estimator>(e), parameters...))
    {
      (*f)(vm, parameters...);
      vm->sp_ -= sp_offset;
    }
  }
};

template <typename ReturnType, typename FreeFunction, typename Estimator, typename... Used>
struct FreeFunctionInvoker
{
  template <int PARAMETER_OFFSET, typename... Ts>
  struct Invoker;
  template <int PARAMETER_OFFSET, typename T, typename... Ts>
  struct Invoker<PARAMETER_OFFSET, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction f, Estimator &&e,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType =
          typename FreeFunctionInvoker<ReturnType, FreeFunction, Estimator, Used...,
                                       T>::template Invoker<PARAMETER_OFFSET - 1, Ts...>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, f, std::forward<Estimator>(e), used...,
                          parameter);
    }
  };
  template <int PARAMETER_OFFSET, typename T>
  struct Invoker<PARAMETER_OFFSET, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction f, Estimator &&e,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType =
          FreeFunctionInvokerHelper<ReturnType, FreeFunction, Estimator, Used..., T>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, f, std::forward<Estimator>(e), used...,
                          parameter);
    }
  };
  template <int PARAMETER_OFFSET>
  struct Invoker<PARAMETER_OFFSET>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction f, Estimator &&e)
    {
      using InvokerType = FreeFunctionInvokerHelper<ReturnType, FreeFunction, Estimator>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, f, std::forward<Estimator>(e));
    }
  };
};

template <typename ReturnType, typename Estimator, typename... Ts>
void InvokeFreeFunction(VM *vm, TypeId return_type_id, ReturnType (*f)(VM *, Ts...), Estimator &&e)
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters - IsResult<ReturnType>::value;
  using FreeFunction                   = ReturnType (*)(VM *, Ts...);
  using FreeFunctionInvoker =
      typename FreeFunctionInvoker<ReturnType, FreeFunction,
                                   Estimator>::template Invoker<first_parameter_offset, Ts...>;
  FreeFunctionInvoker::Invoke(vm, sp_offset, return_type_id, f, std::forward<Estimator>(e));
}

}  // namespace vm
}  // namespace fetch
