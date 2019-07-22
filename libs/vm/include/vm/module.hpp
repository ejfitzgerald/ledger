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

#include "meta/tuple.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/address.hpp"
#include "vm/array.hpp"
#include "vm/compiler.hpp"
#include "vm/map.hpp"
#include "vm/matrix.hpp"
#include "vm/module/base.hpp"
#include "vm/module/constructor_invoke.hpp"
#include "vm/module/free_function_invoke.hpp"
#include "vm/module/functor_invoke.hpp"
#include "vm/module/member_function_invoke.hpp"
#include "vm/module/static_member_function_invoke.hpp"
#include "vm/state.hpp"
#include "vm/vm.hpp"

#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

namespace fetch {
namespace vm {

namespace details {
template <typename T, typename... Ts>
struct CreateSerializeConstructor
{
  static DefaultConstructorHandler Apply()
  {
    return [](VM *vm, TypeId) -> Ptr<Object> {
      vm->RuntimeError("No support for non-default constructors.");
      return nullptr;
    };
  }

  static DefaultConstructorHandler Apply(Ts... args)
  {
    return [args...](VM *vm, TypeId id) -> Ptr<Object> { return T::Constructor(vm, id, args...); };
  }
};

template <typename T>
struct CreateSerializeConstructor<T>
{
  static DefaultConstructorHandler Apply()
  {
    return [](VM *vm, TypeId id) -> Ptr<Object> { return T::Constructor(vm, id); };
  }
};

template <>
struct CreateSerializeConstructor<String>
{
  static DefaultConstructorHandler Apply()
  {
    return [](VM *vm, TypeId /*id*/) -> Ptr<Object> { return new String(vm, ""); };
  }
};

}  // namespace details

class Module
{
public:
  Module();
  ~Module() = default;

  template <typename Type>
  class ClassInterface
  {
  public:
    ClassInterface(Module *module__, TypeIndex type_index__)
      : module_(module__)
      , type_index_(type_index__)
    {}

    template <typename... Ts>
    ClassInterface &CreateConstuctor(ChargeEstimator<Ts...> &&estimator)
    {
      return InternalCreateConstuctor<Ts...>(std::forward<ChargeEstimator<Ts...>>(estimator), 0);
    }

    template <typename... Ts>
    ClassInterface &CreateConstuctor(ChargeAmount static_charge)
    {
      return InternalCreateConstuctor<Ts...>({}, static_charge);
    }

    template <typename... Ts>
    ClassInterface &CreateSerializeDefaultConstuctor(Ts... args)
    {
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollTypes<Ts...>::Unroll(parameter_type_index_array);

      DefaultConstructorHandler h =
          details::CreateSerializeConstructor<Type, Ts...>::Apply(std::forward<Ts>(args)...);
      module_->deserialization_constructors_.insert({type_index__, std::move(h)});

      return *this;
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateStaticMemberFunction(std::string const &function_name,
                                               ReturnType (*f)(VM *, TypeId, Ts...),
                                               ChargeEstimator<Ts...> &&estimator)
    {
      return InternalCreateStaticMemberFunction<ReturnType, Ts...>(
          function_name, f, std::forward<ChargeEstimator<Ts...>>(estimator), 0);
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateStaticMemberFunction(std::string const &function_name,
                                               ReturnType (*f)(VM *, TypeId, Ts...),
                                               ChargeAmount static_charge)
    {
      return InternalCreateStaticMemberFunction<ReturnType, Ts...>(function_name, f, {},
                                                                   static_charge);
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateMemberFunction(std::string const &name, ReturnType (Type::*f)(Ts...),
                                         ChargeEstimator<Ts...> &&e)
    {
      return InternalCreateMemberFunction<ReturnType, decltype(f), Ts...>(
          name, f, std::forward<ChargeEstimator<Ts...>>(e), 0);
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateMemberFunction(std::string const &name, ReturnType (Type::*f)(Ts...),
                                         ChargeAmount       static_charge)
    {
      return InternalCreateMemberFunction<ReturnType, decltype(f), Ts...>(name, f, {},
                                                                          static_charge);
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateMemberFunction(std::string const &name,
                                         ReturnType (Type::*f)(Ts...) const,
                                         ChargeEstimator<Ts...> &&e)
    {
      return InternalCreateMemberFunction<ReturnType, decltype(f), Ts...>(
          name, f, std::forward<ChargeEstimator<Ts...>>(e), 0);
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateMemberFunction(std::string const &name,
                                         ReturnType (Type::*f)(Ts...) const,
                                         ChargeAmount const charge)
    {
      return InternalCreateMemberFunction<ReturnType, decltype(f), Ts...>(name, f, {}, charge);
    }

    ClassInterface &EnableOperator(Operator op)
    {
      TypeIndex const type_index__            = type_index_;
      auto            compiler_setup_function = [type_index__, op](Compiler *compiler) {
        compiler->EnableOperator(type_index__, op);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    // input type 1, input type 2 ... input type N, output type
    template <typename... Types>
    ClassInterface &EnableIndexOperator(
        ChargeEstimator<Types...> &&getter_estimator,
        ChargeEstimator<Types...> &&setter_estimator)  // order changed!
    {
      return InternalEnableIndexOperator<Types...>(
          std::forward<ChargeEstimator<Types...>>(getter_estimator),
          std::forward<ChargeEstimator<Types...>>(setter_estimator), 0, 0);
    }

    // input type 1, input type 2 ... input type N, output type
    template <typename... Types>
    ClassInterface &EnableIndexOperator(ChargeAmount get_static_charge,
                                        ChargeAmount set_static_charge)  // order changed!
    {
      return InternalEnableIndexOperator<Types...>({}, {}, get_static_charge, set_static_charge);
    }

    template <typename InstantiationType>
    ClassInterface &CreateInstantiationType()
    {
      TypeIndex const instantiation_type_index = TypeIndex(typeid(InstantiationType));
      TypeIndex const template_type_index      = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollTemplateParameters<InstantiationType>::Unroll(parameter_type_index_array);
      auto compiler_setup_function = [instantiation_type_index, template_type_index,
                                      parameter_type_index_array](Compiler *compiler) {
        compiler->CreateInstantiationType(instantiation_type_index, template_type_index,
                                          parameter_type_index_array);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);

      return *this;
    }

  private:
    template <typename... Types>
    ClassInterface &InternalEnableIndexOperator(ChargeEstimator<Types...> &&getter_estimator,
                                                ChargeEstimator<Types...> &&setter_estimator,
                                                ChargeAmount                get_charge,
                                                ChargeAmount set_charge)  // order changed!
    {
      static_assert(sizeof...(Types) >= 2, "2 or more types expected");
      using Tuple       = std::tuple<Types...>;
      using InputsTuple = typename meta::RemoveLastType<Tuple>::type;
      using OutputType  = typename meta::GetLastType<Tuple>::type;
      using Getter      = typename IndexedValueGetter<Type, InputsTuple, OutputType>::type;
      using Setter      = typename IndexedValueSetter<Type, InputsTuple, OutputType>::type;
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  input_type_index_array;
      UnrollTypes<Types...>::Unroll(input_type_index_array);
      TypeIndex const output_type_index = input_type_index_array.back();
      input_type_index_array.pop_back();
      Getter  gf          = &Type::GetIndexedValue;
      Handler get_handler = [gf, getter_estimator](VM *vm) {
        InvokeMemberFunction(vm, vm->instruction_->type_id, gf, getter_estimator);
      };
      Setter  sf          = &Type::SetIndexedValue;
      Handler set_handler = [sf, setter_estimator](VM *vm) {
        InvokeMemberFunction(vm, vm->instruction_->type_id, sf, setter_estimator);
      };
      auto compiler_setup_function = [type_index__, input_type_index_array, output_type_index,
                                      get_handler, set_handler, get_charge,
                                      set_charge](Compiler *compiler) {
        compiler->EnableIndexOperator(type_index__, input_type_index_array, output_type_index,
                                      get_handler, set_handler, get_charge, set_charge);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &InternalCreateStaticMemberFunction(std::string const &function_name,
                                                       ReturnType (*f)(VM *, TypeId, Ts...),
                                                       ChargeEstimator<Ts...> &&estimator,
                                                       ChargeAmount             static_charge)
    {
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollParameterTypes<Ts...>::Unroll(parameter_type_index_array);
      TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();
      Handler         handler           = [f, estimator](VM *vm) {
        InvokeStaticMemberFunction(vm, vm->instruction_->data, vm->instruction_->type_id, f,
                                   estimator);
      };
      auto compiler_setup_function = [type_index__, function_name, parameter_type_index_array,
                                      return_type_index, handler,
                                      static_charge](Compiler *compiler) {
        compiler->CreateStaticMemberFunction(type_index__, function_name,
                                             parameter_type_index_array, return_type_index, handler,
                                             static_charge);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    template <typename... Ts>
    ClassInterface &InternalCreateConstuctor(ChargeEstimator<Ts...> &&estimator,
                                             ChargeAmount             static_charge)
    {
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollTypes<Ts...>::Unroll(parameter_type_index_array);
      Handler handler = [estimator](VM *vm) {
        InvokeConstructor<Type, Ts...>(vm, vm->instruction_->type_id, estimator);
      };
      auto compiler_setup_function = [type_index__, parameter_type_index_array, handler,
                                      static_charge](Compiler *compiler) {
        compiler->CreateConstructor(type_index__, parameter_type_index_array, handler,
                                    static_charge);
      };

      module_->AddCompilerSetupFunction(compiler_setup_function);

      if (sizeof...(Ts) == 0)
      {
        DefaultConstructorHandler h = details::CreateSerializeConstructor<Type, Ts...>::Apply();
        module_->deserialization_constructors_.insert({type_index__, std::move(h)});
      }

      return *this;
    }

    template <typename ReturnType, typename MemberFunction, typename... Ts>
    ClassInterface &InternalCreateMemberFunction(std::string const &function_name, MemberFunction f,
                                                 ChargeEstimator<Ts...> &&estimator,
                                                 vm::ChargeAmount         static_charge)
    {
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollParameterTypes<Ts...>::Unroll(parameter_type_index_array);
      TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();
      Handler         handler           = [f, estimator](VM *vm) {
        InvokeMemberFunction(vm, vm->instruction_->type_id, f, estimator);
      };
      auto compiler_setup_function = [type_index__, function_name, parameter_type_index_array,
                                      return_type_index, handler,
                                      static_charge](Compiler *compiler) {
        compiler->CreateMemberFunction(type_index__, function_name, parameter_type_index_array,
                                       return_type_index, handler, static_charge);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    Module *  module_;
    TypeIndex type_index_;
  };

  template <typename Functor, typename... Ts>
  void CreateFreeFunctor(std::string const &name, Functor &&f, ChargeEstimator<Ts...> &&estimator)
  {
    InternalCreateFreeFunctor<Functor, Ts...>(name, std::forward<Functor>(f), estimator, 0);
  }

  template <typename Functor>
  void CreateFreeFunctor(std::string const &name, Functor &&f, ChargeAmount charge)
  {
    InternalCreateFreeFunctor<Functor>(name, std::forward<Functor>(f), {}, charge);
  }

  template <typename ReturnType, typename... Ts>
  void CreateFreeFunction(std::string const &      name, ReturnType (*f)(VM *, Ts...),
                          ChargeEstimator<Ts...> &&estimator)
  {
    InternalCreateFreeFunction<ReturnType, Ts...>(name, f, estimator, 0);
  }

  template <typename ReturnType, typename... Ts>
  void CreateFreeFunction(std::string const &name, ReturnType (*f)(VM *, Ts...),
                          ChargeAmount       charge)
  {
    InternalCreateFreeFunction<ReturnType, Ts...>(name, f, {}, charge);
  }

  template <typename Type>
  ClassInterface<Type> CreateClassType(std::string const &name)
  {
    TypeIndex const type_index              = TypeIndex(typeid(Type));
    auto            compiler_setup_function = [name, type_index](Compiler *compiler) {
      compiler->CreateClassType(name, type_index);
    };
    AddCompilerSetupFunction(compiler_setup_function);
    return ClassInterface<Type>(this, type_index);
  }

  template <typename Type>
  ClassInterface<Type> GetClassInterface()
  {
    TypeIndex const type_index = TypeIndex(typeid(Type));
    return ClassInterface<Type>(this, type_index);
  }

private:
  /*
  // Create a free function that takes an Int32 and a Float32 and returns a Float64
  auto handler = [](fetch::vm::VM*, int32_t i, float f)
  {
    return double(float(i) + f);
  };
  auto estimator = [](fetch::vm::VM*, int32_t i, float f) -> ChargeAmount
  {
    return double(float(i) + f);
  };
  module.CreateFreeFunction("myfunc", std::move(handler), std::move(estimator));
  */
  template <typename Functor, typename... Ts>
  void InternalCreateFreeFunctor(std::string const &name, Functor &&functor,
                                 ChargeEstimator<Ts...> &&estimator, ChargeAmount static_charge)
  {
    using ReturnType     = typename FunctorTraits<Functor>::return_type;
    using SignatureTuple = typename FunctorTraits<Functor>::args_tuple_type;
    using ParameterTuple = typename meta::RemoveFirstType<SignatureTuple>::type;
    TypeIndexArray parameter_type_index_array;
    UnrollTupleParameterTypes<ParameterTuple>::Unroll(parameter_type_index_array);
    TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();

    Handler handler = [f{std::forward<Functor>(functor)}, estimator](VM *vm) {
      InvokeFunctor(vm, vm->instruction_->type_id, f, estimator, ParameterTuple());
    };
    auto compiler_setup_function = [name, parameter_type_index_array, return_type_index, handler,
                                    static_charge](Compiler *compiler) {
      compiler->CreateFreeFunction(name, parameter_type_index_array, return_type_index, handler,
                                   static_charge);
    };
    AddCompilerSetupFunction(compiler_setup_function);
  }

  template <typename ReturnType, typename... Ts>
  void InternalCreateFreeFunction(std::string const &      name, ReturnType (*f)(VM *, Ts...),
                                  ChargeEstimator<Ts...> &&estimator, ChargeAmount static_charge)
  {
    TypeIndexArray parameter_type_index_array;
    UnrollParameterTypes<Ts...>::Unroll(parameter_type_index_array);
    TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();
    Handler         handler           = [f, estimator](VM *vm) {
      InvokeFreeFunction(vm, vm->instruction_->type_id, f, estimator);
    };
    auto compiler_setup_function = [name, parameter_type_index_array, return_type_index, handler,
                                    static_charge](Compiler *compiler) {
      compiler->CreateFreeFunction(name, parameter_type_index_array, return_type_index, handler,
                                   static_charge);
    };
    AddCompilerSetupFunction(compiler_setup_function);
  }

  void CompilerSetup(Compiler *compiler)
  {
    for (auto const &compiler_setup_function : compiler_setup_functions_)
    {
      compiler_setup_function(compiler);
    }
    compiler->GetDetails(type_info_array_, type_info_map_, registered_types_, function_info_array_);
  }

  void GetDetails(TypeInfoArray &type_info_array, TypeInfoMap &type_info_map,
                  RegisteredTypes &registered_types, FunctionInfoArray &function_info_array,
                  DeserializeConstructorMap &deserialization_constructors)
  {
    type_info_array              = type_info_array_;
    type_info_map                = type_info_map_;
    registered_types             = registered_types_;
    function_info_array          = function_info_array_;
    deserialization_constructors = deserialization_constructors_;
  }

  using CompilerSetupFunction = std::function<void(Compiler *)>;

  void AddCompilerSetupFunction(CompilerSetupFunction const &function)
  {
    compiler_setup_functions_.push_back(function);
  }

  std::vector<CompilerSetupFunction> compiler_setup_functions_;
  TypeInfoArray                      type_info_array_;
  TypeInfoMap                        type_info_map_;
  RegisteredTypes                    registered_types_;
  FunctionInfoArray                  function_info_array_;
  DeserializeConstructorMap          deserialization_constructors_;

  friend class Compiler;
  friend class VM;
};

}  // namespace vm
}  // namespace fetch
